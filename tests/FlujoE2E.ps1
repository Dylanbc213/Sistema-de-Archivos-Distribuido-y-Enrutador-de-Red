$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$runDirectory = Join-Path $tempRoot ('network-os-e2e-' + [guid]::NewGuid().ToString('N'))
[System.IO.Directory]::CreateDirectory($runDirectory) | Out-Null
$pushedLocation = $false

function Assert-Contains([string]$text, [string]$expected, [string]$description) {
    if (-not $text.Contains($expected)) {
        throw "Fallo E2E: $description (no se encontro '$expected')."
    }
}

function Invoke-NetworkOS([string[]]$lines) {
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $script:appPath
    $startInfo.WorkingDirectory = $script:runDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    if (-not $process.Start()) { throw 'No se pudo iniciar NetworkOS.exe.' }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    foreach ($line in $lines) { $process.StandardInput.WriteLine($line) }
    $process.StandardInput.Close()
    if (-not $process.WaitForExit(30000)) {
        $process.Kill()
        throw 'NetworkOS.exe excedio el limite de 30 segundos.'
    }
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    $exitCode = $process.ExitCode
    $process.Dispose()
    return [pscustomobject]@{ Output = $stdout; Error = $stderr; ExitCode = $exitCode }
}

try {
    Push-Location $repoRoot
    $pushedLocation = $true

    $previousAdminUser = $env:NETWORK_OS_ADMIN_USER
    $previousAdminPassword = $env:NETWORK_OS_ADMIN_PASSWORD
    $previousLocalAppData = $env:LOCALAPPDATA
    $env:NETWORK_OS_ADMIN_USER = 'tester'
    $env:NETWORK_OS_ADMIN_PASSWORD = 'clave-segura'
    $env:LOCALAPPDATA = $runDirectory

    $appPath = Join-Path $runDirectory 'NetworkOS.exe'
    $sources = @(
        'main.cpp', 'Archivo.cpp', 'Auditoria.cpp', 'Carpeta.cpp',
        'ConexionRed.cpp', 'GrafoRed.cpp', 'LoggerAuditoria.cpp', 'Menu.cpp',
        'NodoArchivo.cpp', 'Servidor.cpp', 'Sistema.cpp',
        'TablaHashUsuarios.cpp', 'Usuario.cpp'
    )
    & g++ -std=c++17 -Wall -Wextra -O2 @sources -lbcrypt -o $appPath
    if ($LASTEXITCODE -ne 0) { throw "La compilacion E2E fallo con codigo $LASTEXITCODE." }

    $firstRun = Invoke-NetworkOS @(
        'texto-invalido', '1', '2',
        '5', 'tester', 'incorrecta', '5', 'no-existe', 'x',
        '5', 'tester', 'clave-segura',
        '2', '1', '33', 'normal', 'normalpass', '0', '-1',
        '1', '34', 'normal', 'duplicada', '0', '-1', '3', 'tester', '0',
        '1', '1', '10', 'norte', '1', '20', 'sur', '0',
        '3', '6', '10', '20', '7', '8', '10', '20', '1', '10', '2', '10', '4', '9', '20', '5', '0',
        '5', '2', '1',
        '5', 'normal', 'normalpass', '2',
        '1', '4', '10', '/docs', '5', '10', '/docs/nota.txt', 'contenido',
        '6', '10', 'nota.txt', '7', '10', '/docs/nota.txt', '3', '10',
        '8', '10', '/docs', '3', '10', '8', '10', '/', 'xyz', '0',
        '3', '1', '10', '4', '0',
        '4', '1', '0', '5', '4', '1', '0'
    )
    if ($firstRun.ExitCode -ne 0) { throw "La primera ejecucion termino con codigo $($firstRun.ExitCode)." }
    Assert-Contains $firstRun.Output 'Administrador inicial persistido: tester (ADMIN)' 'provision persistente del administrador inicial'
    Assert-Contains $firstRun.Output 'Bienvenido, "tester" (ADMIN)' 'autenticacion administrativa desde el menu'
    Assert-Contains $firstRun.Output 'Usuario "normal" registrado' 'creacion de usuario por ADMIN'
    Assert-Contains $firstRun.Output 'No se pudo registrar el usuario' 'rechazo de cuenta duplicada'
    Assert-Contains $firstRun.Output 'No se pudo eliminar el usuario' 'proteccion de la cuenta administrativa activa'
    Assert-Contains $firstRun.Output 'Bienvenido, "normal" (NORMAL)' 'autenticacion como usuario normal'
    Assert-Contains $firstRun.Output 'Se requiere una sesion ADMIN para acceder a gestion de usuarios' 'bloqueo de gestion de usuarios para NORMAL'
    Assert-Contains $firstRun.Output 'Inicie sesion para acceder a gestion de usuarios' 'bloqueo de gestion de usuarios sin sesion'
    Assert-Contains $firstRun.Output '5. Iniciar sesion' 'opcion visible de inicio de sesion sin sesion activa'
    Assert-Contains $firstRun.Output 'El usuario "no-existe" no existe' 'mensaje de usuario inexistente'
    Assert-Contains $firstRun.Output 'Contrasena incorrecta para el usuario "tester"' 'rechazo de credenciales incorrectas'
    Assert-Contains $firstRun.Output '5. Cerrar sesion (tester)' 'opcion visible para cerrar sesion'
    Assert-Contains $firstRun.Output 'Sesion cerrada para "tester"' 'cierre de sesion desde menu principal'
    Assert-Contains $firstRun.Output 'Inicie sesion para acceder a gestion de archivos' 'bloqueo de archivos tras logout'
    Assert-Contains $firstRun.Output 'Servidor #10' 'creacion del servidor desde el menu'
    Assert-Contains $firstRun.Output 'Conexion dirigida agregada' 'alta de enlace desde el menu'
    Assert-Contains $firstRun.Output 'latencia total: 7 ms' 'ruta Dijkstra desde el menu'
    Assert-Contains $firstRun.Output 'Recorrido BFS: #10 norte -> #20 sur' 'BFS desde el menu'
    Assert-Contains $firstRun.Output 'Recorrido DFS: #10 norte -> #20 sur' 'DFS desde el menu'
    Assert-Contains $firstRun.Output 'PING OK #10 -> #20' 'ping conectado'
    Assert-Contains $firstRun.Output 'PING FALLIDO #20 -> #10' 'ping sin ruta dirigida'
    Assert-Contains $firstRun.Output 'Servidor eliminado de la topologia de red' 'baja de un servidor de la topologia'
    Assert-Contains $firstRun.Output 'Grafo de red (1 servidores, 0 aristas)' 'consistencia del grafo tras la baja'
    Assert-Contains $firstRun.Output 'Archivo creado: /docs/nota.txt' 'creacion de archivo'
    Assert-Contains $firstRun.Output 'Elemento eliminado: /docs' 'eliminacion recursiva'
    Assert-Contains $firstRun.Output 'No se puede eliminar la raiz' 'rechazo de borrar raiz'
    Assert-Contains $firstRun.Output 'Opcion invalida en el submenu "Gestion de archivos"' 'rechazo de entrada de submenú inválida'
    Assert-Contains $firstRun.Output 'Opcion invalida. Ingrese un numero del menu' 'rechazo de entrada principal inválida'
    Assert-Contains $firstRun.Output 'Inicie sesion para acceder a auditoria' 'acceso a auditoria bloqueado sin sesion'

    $dataDirectory = Join-Path $runDirectory 'NetworkOS'
    $networkLogPath = Join-Path $dataDirectory 'network_audit_log.txt'
    $auditLogPath = Join-Path $dataDirectory 'auditoria.log'
    if (-not (Test-Path -LiteralPath $networkLogPath)) { throw 'No se creo el logger persistente.' }
    if (-not (Test-Path -LiteralPath $auditLogPath)) { throw 'No se guardo la auditoria de la sesion.' }
    $persistentLog = Get-Content -LiteralPath $networkLogPath -Raw
    Assert-Contains $persistentLog '<USUARIO> Administrador inicial provisionado: "tester".' 'evento persistente del admin inicial'
    Assert-Contains $persistentLog '<USUARIO> Usuario "normal" registrado' 'evento persistente de usuario'
    Assert-Contains $persistentLog '<PING> PING OK #10 -> #20' 'evento persistente de ping'
    Assert-Contains $persistentLog '<ACCESS_DENIED>' 'registro de acceso denegado'
    if ($persistentLog -notmatch '\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\]') {
        throw 'Los eventos persistentes no tienen fecha y hora con el formato esperado.'
    }

    $secondRun = Invoke-NetworkOS @(
        '5', 'tester', 'clave-segura', '2', '2', '0',
        '1', '1', '30', 'persistente', '4', '30', '/durable/docs',
        '5', '30', '/durable/docs/state.txt', 'payload-persistente',
        '9', '30', '/durable/docs/state.txt', 'contenido-modificado', '0',
        '3', '6', '10', '30', '7', '8', '10', '30', '1', '10', '2', '10', '0',
        '5', '5', 'normal', 'normalpass', '1', '3', '30', '0',
        '3', '1', '10', '2', '10', '8', '10', '30', '0', '0'
    )
    if ($secondRun.ExitCode -ne 0) { throw "La segunda ejecucion termino con codigo $($secondRun.ExitCode)." }
    Assert-Contains $secondRun.Output 'Se reutilizaron las cuentas ADMIN guardadas' 'reutilizacion del admin al reiniciar'
    Assert-Contains $secondRun.Output 'TablaHash de usuarios (2 elementos' 'conservar los usuarios guardados sin duplicarlos'
    Assert-Contains $secondRun.Output 'latencia total: 7 ms' 'Dijkstra sobre la topologia persistida'
    Assert-Contains $secondRun.Output 'Recorrido BFS: #10 norte -> #30 persistente' 'BFS sobre grafo persistido'
    Assert-Contains $secondRun.Output 'Recorrido DFS: #10 norte -> #30 persistente' 'DFS sobre grafo persistido'
    Assert-Contains $secondRun.Output '[FILE] state.txt' 'arbol disponible despues de cambiar de usuario'
    Assert-Contains $secondRun.Output 'Contenido actualizado y guardado' 'modificacion de archivo guardada'
    Assert-Contains $secondRun.Output 'Bienvenido, "normal" (NORMAL)' 'cambio de sesion sin limpiar el estado'
    if (-not (Test-Path -LiteralPath (Join-Path $dataDirectory 'sistema.db'))) {
        throw 'No se creo el archivo persistente del estado del sistema.'
    }

    $thirdRun = Invoke-NetworkOS @(
        '5', 'normal', 'normalpass', '3', '1', '10', '2', '10', '8', '10', '30',
        '7', '10', '30', '8', '10', '30', '0', '0'
    )
    if ($thirdRun.ExitCode -ne 0) { throw "La tercera ejecucion termino con codigo $($thirdRun.ExitCode)." }
    Assert-Contains $thirdRun.Output 'Recorrido BFS: #10 norte -> #30 persistente' 'BFS disponible en la ejecucion reiniciada'
    Assert-Contains $thirdRun.Output 'Conexion eliminada' 'eliminacion intencional de enlace'
    Assert-Contains $thirdRun.Output 'No hay ruta entre #10 y #30' 'Dijkstra refleja enlace eliminado'

    $fourthRun = Invoke-NetworkOS @(
        '5', 'tester', 'clave-segura', '3', '5', '8', '10', '30', '0',
        '1', '3', '30', '0', '0'
    )
    if ($fourthRun.ExitCode -ne 0) { throw "La cuarta ejecucion termino con codigo $($fourthRun.ExitCode)." }
    Assert-Contains $fourthRun.Output 'Grafo de red (2 servidores, 0 aristas)' 'baja de conexion sobrevive al reinicio'
    Assert-Contains $fourthRun.Output 'No hay ruta entre #10 y #30' 'ruta eliminada sigue sin estar disponible tras reinicio'
    Assert-Contains $fourthRun.Output '[FILE] state.txt' 'servidor y archivo siguen disponibles tras reinicio'
    $persistentAfterRestart = Get-Content -LiteralPath $networkLogPath -Raw
    Assert-Contains $persistentAfterRestart '<PING> PING OK #10 -> #20' 'persistencia del log entre ejecuciones'

    Write-Output 'E2E_OK: usuarios (regresion), archivos, servidores, topologia, BFS, DFS, Dijkstra, bajas y auditoria persistente.'
    Write-Output 'Estado global y logs: LOCALAPPDATA\NetworkOS; usuarios permanecen en usuarios.db.'
}
finally {
    if ($null -eq $previousAdminUser) { Remove-Item Env:NETWORK_OS_ADMIN_USER -ErrorAction SilentlyContinue }
    else { $env:NETWORK_OS_ADMIN_USER = $previousAdminUser }
    if ($null -eq $previousAdminPassword) { Remove-Item Env:NETWORK_OS_ADMIN_PASSWORD -ErrorAction SilentlyContinue }
    else { $env:NETWORK_OS_ADMIN_PASSWORD = $previousAdminPassword }
    if ($null -eq $previousLocalAppData) { Remove-Item Env:LOCALAPPDATA -ErrorAction SilentlyContinue }
    else { $env:LOCALAPPDATA = $previousLocalAppData }
    if ($pushedLocation) { Pop-Location }
    $resolvedTempRoot = [System.IO.Path]::GetFullPath($tempRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    $resolvedRunDirectory = [System.IO.Path]::GetFullPath($runDirectory)
    if ($resolvedRunDirectory.StartsWith($resolvedTempRoot, [System.StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $resolvedRunDirectory).StartsWith('network-os-e2e-', [System.StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $resolvedRunDirectory -Recurse -Force -ErrorAction SilentlyContinue
    }
}
