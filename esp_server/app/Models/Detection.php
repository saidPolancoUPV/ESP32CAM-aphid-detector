<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Detection extends Model
{
    protected $fillable = [
        'image_path',
        'modified_image_path',
        'detections_data',
        'timestamp'
    ];

    public function getOriginalImageUrlAttribute() : string
    {
        return asset('storage/' . $this->image_path);
    }

    public function getModifiedImageUrlAttribute() : string
    {
        return $this->modified_image_path ? asset('storage/' . $this->modified_image_path) : null;
    }
}
