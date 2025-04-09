<?php
namespace App\Livewire;

use Livewire\Component;
use App\Models\Detection;

class DetectionList extends Component
{
    public $detections;

    protected $listeners = ['detectionAdded' => 'refreshDetections'];

    public function mount()
    {
        $this->refreshDetections();
    }

    public function refreshDetections()
    {
        $this->detections = Detection::whereNotNull('detections_data')
            ->latest()
            ->get();
    }

    public function deleteDetection($id)
    {
        $detection = Detection::findOrFail($id);
        $detection->delete();
        $this->refreshDetections();
    }

    public function render()
    {
        return view('livewire.detection-list');
    }
}
