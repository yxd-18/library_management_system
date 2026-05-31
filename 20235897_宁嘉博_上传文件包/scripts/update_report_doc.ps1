$ErrorActionPreference = 'Stop'

$src = 'D:\WeChat_files\WeChat Files\wxid_q8ndugwzao5a22\FileStorage\File\2026-05\图书管理系统研发文档.docx'
$dst = 'C:\Users\袁小迪\OneDrive\桌面\library_management_system\图书管理系统研发文档_完善版.docx'
$base = 'C:\Users\袁小迪\OneDrive\桌面\library_management_system\doc_sections'

Copy-Item $src $dst -Force

$word = $null
$doc = $null

function Replace-BetweenText {
    param(
        $doc,
        [string]$startText,
        [string]$endText,
        [string]$newText
    )

    $startRange = $doc.Content.Duplicate
    $startFind = $startRange.Find
    $startFind.ClearFormatting()
    $startFind.Text = $startText
    $startFind.Forward = $true
    $startFind.Wrap = 0
    if (-not $startFind.Execute()) {
        throw "Start text not found: $startText"
    }

    $searchRange = $doc.Range($startRange.End, $doc.Content.End)
    $endFind = $searchRange.Find
    $endFind.ClearFormatting()
    $endFind.Text = $endText
    $endFind.Forward = $true
    $endFind.Wrap = 0
    if (-not $endFind.Execute()) {
        throw "End text not found: $endText"
    }

    $replaceRange = $doc.Range($startRange.End, $searchRange.Start)
    $replaceRange.Text = "`r" + $newText.Trim() + "`r"
}

$sec115 = Get-Content (Join-Path $base 'sec115.txt') -Raw
$sec215 = Get-Content (Join-Path $base 'sec215.txt') -Raw
$sec31 = Get-Content (Join-Path $base 'sec31.txt') -Raw
$sec4 = Get-Content (Join-Path $base 'sec4.txt') -Raw
$sec5 = Get-Content (Join-Path $base 'sec5.txt') -Raw

try {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $doc = $word.Documents.Open($dst)

    Replace-BetweenText -doc $doc -startText '1.1.5人员安排' -endText '1.2 任务概述' -newText $sec115
    Replace-BetweenText -doc $doc -startText '2.1.5人员安排' -endText '2.2 系统功能设计' -newText $sec215
    Replace-BetweenText -doc $doc -startText '3.1人员安排' -endText '3.2 系统通用类实现' -newText $sec31
    Replace-BetweenText -doc $doc -startText '4.1人员安排' -endText '5 心得体会' -newText $sec4

    $startRange = $doc.Content.Duplicate
    $startFind = $startRange.Find
    $startFind.ClearFormatting()
    $startFind.Text = '5 心得体会'
    $startFind.Forward = $true
    $startFind.Wrap = 0
    if (-not $startFind.Execute()) {
        throw 'Start text not found: 5 心得体会'
    }

    $tail = $doc.Range($startRange.End, $doc.Content.End)
    $tail.Text = "`r" + $sec5.Trim() + "`r"

    $doc.Fields.Update() | Out-Null
    $doc.Save()
    Write-Output "UPDATED=$dst"
}
catch {
    Write-Error $_
    exit 1
}
finally {
    if ($doc -ne $null) { $doc.Close() }
    if ($word -ne $null) { $word.Quit() }
}
