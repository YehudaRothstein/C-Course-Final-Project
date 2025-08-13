Param([string]$Root = ".")
Set-Location $Root
Get-ChildItem -Path 'src' -Recurse -File -Include *.c,*.h | ForEach-Object {
  $p = $_.FullName
  $t = Get-Content -LiteralPath $p -Raw -ErrorAction SilentlyContinue
  if ($null -ne $t) {
    $u = [System.Text.RegularExpressions.Regex]::Replace($t, '/\*.*?\*/', '', [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if ($u -ne $t) {
      Set-Content -LiteralPath $p -Value $u -NoNewline
    }
  }
}
