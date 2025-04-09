<!-- resources/views/livewire/detection-list.blade.php -->
<div>
    <div class="container mx-auto px-4 py-6">
        <h1 class="text-2xl font-bold mb-6">Detecciones</h1>

        <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-6">
            @foreach($detections as $detection)
                <div class="bg-white shadow-md rounded-xl overflow-hidden relative">
                    <img src="{{ $detection->original_image_url }}" alt="Detección" class="w-full h-48 object-cover">
                    <div class="p-4">
                        <div class="flex justify-between items-start mb-2">
                            <h2 class="text-lg font-semibold">Detección #{{ $detection->id }}</h2>
                            <div class="flex items-center gap-2">
                                @if($detection->modified_image_url)
                                    <button onclick="openModal({{ $detection->id }})" class="text-blue-500 hover:text-blue-700" title="Ver imagen modificada">
                                        🖼️
                                    </button>
                                @endif

                                <button wire:click="deleteDetection({{ $detection->id }})"
                                        onclick="return confirm('¿Estás seguro de que quieres eliminar esta detección?')"
                                        class="text-red-500 hover:text-red-700" title="Eliminar">
                                    <svg xmlns="http://www.w3.org/2000/svg" class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
                                    </svg>
                                </button>
                            </div>
                        </div>
                        <p class="text-sm text-gray-600 mb-2">Fecha: {{ \Carbon\Carbon::parse($detection->timestamp)->format('d/m/Y H:i') }}</p>
                        <p class="text-sm text-gray-700 break-words">
                            <strong>Datos:</strong> {{ $detection->detections_data }}
                        </p>
                    </div>
                </div>

                @if($detection->modified_image_url)
                    <div id="modal-{{ $detection->id }}" class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50 hidden">
                        <div class="bg-white rounded-lg shadow-lg max-w-3xl w-full relative">
                            <button onclick="closeModal({{ $detection->id }})" class="absolute top-2 right-2 text-gray-500 hover:text-black text-xl">&times;</button>
                            <img src="{{ $detection->modified_image_url }}" alt="Imagen modificada" class="w-full h-auto rounded-lg">
                            <div class="p-4 text-center">
                                <p class="text-sm text-gray-600">Imagen con detecciones resaltadas</p>
                            </div>
                        </div>
                    </div>
                @endif
            @endforeach
        </div>

        @if($detections->isEmpty())
            <p class="text-gray-500 text-center mt-6">No hay detecciones válidas registradas.</p>
        @endif
    </div>

    <script>
        function openModal(id) {
            document.getElementById('modal-' + id).classList.remove('hidden');
        }
        function closeModal(id) {
            document.getElementById('modal-' + id).classList.add('hidden');
        }
    </script>
</div>
