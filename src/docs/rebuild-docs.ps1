if( "$Env:GITVERSION_NUGETVERSION" -eq "" )
{
	$ver = &{gitversion /output json /showvariable FullSemVer}
	Write-Host "Using local gitversion..$ver"
}
else
{
	Write-Host "Using environment $Env:GITVERSION_NUGETVERSION.."
	$ver = $Env:GITVERSION_NUGETVERSION
}

Write-Host "Using version $ver to update the docs.."
&{ type .\Doxyfile ; echo "PROJECT_NUMBER=$ver" } | doxygen -