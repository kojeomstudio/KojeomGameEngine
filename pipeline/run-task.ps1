param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('content', 'document')]
    [string]$Task
)

chcp 65001 > $null

$envVarName = "prompt_$Task"
$promptInput = [System.Environment]::GetEnvironmentVariable($envVarName)

Write-Host "Starting Kilo CLI execution for task: $Task"

if ([string]::IsNullOrWhiteSpace($promptInput)) {
    Write-Error "Error: Environment variable '$envVarName' is not set."
    exit 1
}

try {
    & kilo run "$promptInput"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Kilo CLI execution failed. Exit Code: $LASTEXITCODE"
        exit $LASTEXITCODE
    }
    Write-Host "Kilo CLI execution completed."
}
catch {
    Write-Error "An exception occurred during script execution: $_"
    exit 1
}
