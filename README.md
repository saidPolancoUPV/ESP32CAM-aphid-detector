# Arduino
El deployment de arduino fue hecho usando _Edge Impulse_, para poder usar el 
código principal es necesario tener instalada la librería `aphid_inferencing`.

Para instalar la librería simplemente hay que copiar la carpeta `aphid_inferencing`
dentro de la carpeta `~/Arduino/libraries/`.

Después de eso solo sería compilar y subir a la ESP32-CAM el código `camera.ino`.

# Servidor
El servidor fue desarrollado en PHP, usando laravel. De preferencia úsese docker
para no tener problemas de incompatibilidad.

## Requisitos
- Docker
- PHP
- Composer
- NodeJS y npm
- Laravel (comando, véase [instalación](https://laravel.com/docs/12.x/installation))

- En caso de no tener Docker:
  - Servidor de MYSQL

## Iniciar el server
Laravel cuenta cocn su propia abstración de docker así que hay que instalarla.

Dentro de la raíz del proyecto:
``` bash
# Abstracción de docker
composer require laravel/sail --dev
php artisan sail:install # solo para instalar 

./vendor/bin/sail up # Inicia el proyecto
```

Una vez iniciado docker hay que correr las migraciones:
```bash
sail artisan migrate
```

_Si se requiere utilizar **artisan** para alguna otra acción, recomiendo usarlo desde
sail (docker) en vez de usar directamente php_.

Credenciales de la base de datos:
- Nombre de la BD: esp_server
- Usuario: root
- Contraseña: password
- tabla principal: detections

## Rutas del servidor
El servidor cuenta con 2 rutas:
- Ruta de la API: /api/detections
  - Esta ruta sirve para acceder al servidor a través de una **POST REQUEST**.
    Por ejemplo:
``` bash
curl -X POST \
      http://localhost/api/detections \
      -H 'Content-Type: multipart/form-data' \
      -F 'image=@/home/ale/Downloads/tndhs2zng4-1/Images/bak/aphid.327.jpg' \
      -F 'detections={"objects":[{"label":"aphid","value":0.95,"x":24,"y":24,"width":8,"height":8}]}'
```

Es importante que dentro de `objects` contenga tanto `label` como `value`, estos valores 
se decodifican del json para obtener las _bounding boxes_ y el porcentaje de la predicción; 
dado que `objects` es un array, este puede contener múltiples items.

- Ruta para visualizar los datos: /detections

**En caso de tener problemas de conectividad entre dispositivos, asegúrese de que tanto
la IP de la ESP32-CAM como la del servidor estén en la misma _subnet_**.