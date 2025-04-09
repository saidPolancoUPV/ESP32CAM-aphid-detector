<?php

namespace App\Http\Controllers;

use App\Models\Detection;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Storage;

class Esp32CamController extends Controller
{

    public function receiveDetectionData(Request $request)
    {
        try {
            $request->validate([
                'image' => 'required|image',
                'detections' => 'required',
            ]);

            $path = $request->file('image')->store('esp32-cam', 'public');
            $detections = json_decode($request->input('detections'), true);
            $detectionsJson = json_encode($detections);

            // Generar nombre de salida
            $modifiedFilename = 'modified_' . basename($path);
            $outputPath = 'esp32-cam/' . $modifiedFilename;

            // Dibujar los bounding boxes
            $this->drawBoundingBoxes($path, $detections, $outputPath);

            // Guardar en base de datos
            Detection::create([
                'image_path' => $path,
                'modified_image_path' => $outputPath,
                'detections_data' => $detectionsJson,
                'timestamp' => time(),
            ]);

            return response()->json([
                'success' => true,
                'message' => 'Data saved and image processed successfully'
            ]);

        } catch (\Exception $e) {
            return response()->json([
                'success' => false,
                'message' => 'Error processing data: ' . $e->getMessage()
            ]);
        }
    }

    public function drawBoundingBoxes($imagePath, $detections, $outputPath)
    {
        $fullPath = storage_path("app/public/" . $imagePath);
        $image = imagecreatefromjpeg($fullPath);

        if (!$image) {
            throw new \Exception("No se pudo abrir la imagen");
        }

        $originalWidth = imagesx($image);
        $originalHeight = imagesy($image);

        // Escala desde 96x96 a original
        $scaleX = $originalWidth / 96;
        $scaleY = $originalHeight / 96;

        $green = imagecolorallocate($image, 0, 255, 0);

        foreach ($detections['objects'] as $box) {
            $x = (int) ($box['x'] * $scaleX);
            $y = (int) ($box['y'] * $scaleY);
            $w = (int) ($box['width'] * $scaleX);
            $h = (int) ($box['height'] * $scaleY);
            $label = $box['label'] . ' ' . $box['value'];

            // Dibujar rectángulo
            imagerectangle($image, $x, $y, $x + $w, $y + $h, $green);
            $textBgColor = imagecolorallocate($image, 0, 128, 0);
            imagefilledrectangle($image, $x, $y - 12, $x + (strlen($label) * 6), $y, $textBgColor);

            // Dibujar texto (label)
            $textColor = imagecolorallocate($image, 255, 255, 255);
            imagestring($image, 1, $x + 1, $y - 11, $label, $textColor);
        }

        $outputFullPath = storage_path("app/public/" . $outputPath);
        imagejpeg($image, $outputFullPath);

        imagedestroy($image);

        return $outputPath;
    }
}
