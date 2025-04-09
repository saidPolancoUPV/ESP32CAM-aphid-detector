#include <aphid_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"

/**
 * Camera configuration for AI Thinker model
 */
#define CAMERA_MODEL_AI_THINKER

// Camera pin definitions
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Network credentials
const char* ssid = "POCO M6";
const char* password = "redacted";
const char* serverUrl = "http://192.168.118.11/api/detections";

// Camera frame configuration
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS   320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS   240
#define EI_CAMERA_FRAME_BYTE_SIZE         3

static bool debug_nn = false;
static bool is_initialised = false;
uint8_t *snapshot_buf;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

/**
 * Initializes the camera module
 * @return true if initialization succeeded, false otherwise
 */
bool ei_camera_init() {
    if (is_initialised) return true;

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }

    is_initialised = true;
    return true;
}

/**
 * Captures an image and processes it for inference
 * @param img_width Target image width
 * @param img_height Target image height
 * @param out_buf Output buffer for processed image
 * @param fb Camera frame buffer
 * @return true if capture and processing succeeded, false otherwise
 */
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf, camera_fb_t* fb) {
    if (!is_initialised) {
        Serial.println("Camera is not initialized");
        return false;
    }

    if (!fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, out_buf)) {
        Serial.println("Conversion failed");
        return false;
    }

    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || 
        (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
    }

    return true;
}

/**
 * Callback function for getting image data during inference
 * @param offset Data offset
 * @param length Length of data to retrieve
 * @param out_ptr Output buffer for the data
 * @return Always returns 0 (success)
 */
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t out_ptr_ix = 0;

    while (length--) {
        out_ptr[out_ptr_ix++] = (snapshot_buf[pixel_ix + 2] << 16) + 
                               (snapshot_buf[pixel_ix + 1] << 8) + 
                               snapshot_buf[pixel_ix];
        pixel_ix += 3;
    }

    return 0;
}

/**
 * Runs object detection inference on a captured image
 * @param fb Camera frame buffer containing the image
 * @return JSON string with detection results or empty string on failure
 */
String runInference(camera_fb_t* fb) {
    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * 
                                  EI_CAMERA_RAW_FRAME_BUFFER_ROWS * 3);
    if (!snapshot_buf) {
        Serial.println("Failed to allocate snapshot buffer");
        return "";
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (!ei_camera_capture(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf, fb)) {
        free(snapshot_buf);
        return "";
    }

    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("Failed to run classifier (%d)\n", err);
        free(snapshot_buf);
        return "";
    }

    String jsonResult;
    bool firstDetection = true;
    
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) continue;

        if (!firstDetection) jsonResult += ",";
        firstDetection = false;
        
        jsonResult += "{\"label\":\"" + String(bb.label) + "\",";
        jsonResult += "\"value\":" + String(bb.value, 5) + ",";
        jsonResult += "\"x\":" + String(bb.x) + ",";
        jsonResult += "\"y\":" + String(bb.y) + ",";
        jsonResult += "\"width\":" + String(bb.width) + ",";
        jsonResult += "\"height\":" + String(bb.height) + "}";
    }

    free(snapshot_buf);
    return firstDetection ? "[]" : "[" + jsonResult + "]";
}

/**
 * Sends image and detection data to the server
 * @param imageData Pointer to the image data
 * @param imageLen Length of the image data
 * @param prediction JSON string with detection results
 */
void sendData(const uint8_t* imageData, size_t imageLen, const char* prediction) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return;
    }

    HTTPClient http;
    http.begin(serverUrl);
    
    String boundary = "----WebKitFormBoundary" + String(random(0xFFFFFFFF), HEX);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    String body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n";
    body += "Content-Type: image/jpeg\r\n\r\n";
    
    String detectionPart = "\r\n--" + boundary + "\r\n";
    detectionPart += "Content-Disposition: form-data; name=\"detections\"\r\n\r\n";
    detectionPart += "{\"objects\":" + String(prediction) + "}\r\n";
    detectionPart += "--" + boundary + "--\r\n";
    
    size_t totalLen = body.length() + imageLen + detectionPart.length();
    uint8_t* postData = (uint8_t*)malloc(totalLen);
    
    if (!postData) {
        Serial.println("Failed to allocate memory");
        return;
    }

    memcpy(postData, body.c_str(), body.length());
    memcpy(postData + body.length(), imageData, imageLen);
    memcpy(postData + body.length() + imageLen, detectionPart.c_str(), detectionPart.length());

    int httpResponseCode = http.POST(postData, totalLen);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode > 0) {
        Serial.print("Server response: ");
        Serial.println(http.getString());
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }

    free(postData);
    http.end();
}

/**
 * Initializes serial communication, WiFi, and camera
 */
void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");

    if (!ei_camera_init()) {
        Serial.println("Failed to initialize camera");
        return;
    }
    Serial.println("Camera initialized");
}

/**
 * Main loop that captures images, runs inference, and sends results
 */
void loop() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Failed to capture image");
        delay(5000);
        return;
    }

    String jsonResult = runInference(fb);
    
    if (jsonResult != "[]") {
        Serial.println("Found objects - sending to server");
        sendData(fb->buf, fb->len, jsonResult.c_str());
    } else {
        Serial.println("No objects detected");
    }
    
    esp_camera_fb_return(fb);
    delay(2000);
}