# 0. 65001 set
chcp 65001

# 1. Get TeamCity environment variable
 $promptInput = $env:prompt_document

# 2. Verify variable value (for debugging - output to build log)
Write-Host "Starting Coding Agent CLI execution."
Write-Host "Received prompt: $promptInput"

# 3. Error handling if the value is empty
if ([string]::IsNullOrWhiteSpace($promptInput)) {
    Write-Error "Error: Environment variable is not set."
    exit 1
}

# 4. Execute Kilo Code CLI
# If the CLI executable is registered in PATH, it can be called directly using 'kilo', etc.

try {
    # Example: kilo run --auto "input_value"
    # Pay attention to how the variable is wrapped for quote handling.
    & kilo run "$promptInput" --auto
    #& claude --permission-mode bypassPermissions -p "$promptInput"
	#& opencode run "$promptInput"
	
    # Check exit code (assuming the CLI returns a non-zero code on failure)
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Kilo Code CLI execution failed. Exit Code: $LASTEXITCODE"
        exit $LASTEXITCODE
    }
    
    Write-Host "Kilo Code CLI execution completed."
}
catch {
    Write-Error "An exception occurred during script execution: $_"
    exit 1
}