<?php

namespace App\Models;

use App\Http\Controllers\Controller;
use Illuminate\Database\Eloquent\Model;

class DetectionController extends Controller
{
    public function index()
    {
        $detections = Detection::orderBy('timestamp', 'desc')->get();
        return view('detections.index', ['detections' => $detections]);
    }

    public function destroy($id)
    {
        Detection::destroy($id); // o Detection::findOrFail($id)->delete();
        return redirect()->route('detections.index')->with('success', 'Detección eliminada correctamente.');
    }
}
