<?php

use App\Models\DetectionController;
use Illuminate\Support\Facades\Route;

Route::get('/', function () {
    return view('welcome');
});

Route::resources([
    'detections' => DetectionController::class,
]);
