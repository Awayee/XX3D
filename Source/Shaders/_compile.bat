for %%i in (*.spv) do (del /f /s /q %%i)
for %%i in (*.vert) do (C:\lib\VulkanSDK\1.2.189.0\Bin\glslc.exe %%i -o %%i.spv)
for %%i in (*.frag) do (C:\lib\VulkanSDK\1.2.189.0\Bin\glslc.exe %%i -o %%i.spv)
pause