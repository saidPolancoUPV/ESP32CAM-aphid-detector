<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Detection Results</title>
    <script src="https://cdn.tailwindcss.com"></script>
    @livewireStyles
    <style>
        .image-placeholder {
            background-color: #f3f4f6;
            display: flex;
            align-items: center;
            justify-content: center;
        }
    </style>
</head>
<body class="bg-gray-50">
    @livewire('detection-list')

    @livewireScripts
</body>
</html>
