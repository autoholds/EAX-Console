"""Generate engine/OctopusEAX/src/engine_core.cpp = app.cpp lines 1..1432 (the audio engine: devices, VB-CABLE, shared state g, audioThread)
verbatim + the export API (scratchpad api_body.cpp / api_header.h). Re-run after pulling a newer app.cpp."""
S = r'C:\Users\weedx\AppData\Local\Temp\claude\c--Users-weedx-Downloads-EAX-UI\90a2e2f8-24c3-4fc5-beed-74abd5363aaa\scratchpad'
R = r'C:\Users\weedx\Downloads\EAX UI\engine\OctopusEAX\src'
src = open(R + r'\app.cpp', encoding='utf-8-sig').read().split('\n')
head = '\n'.join(src[:1432])
open(R + r'\engine_core.cpp', 'w', encoding='utf-8', newline='').write(head + open(S + r'\api_body.cpp', encoding='utf-8').read())
open(R + r'\engine_api.h', 'w', encoding='utf-8', newline='').write(open(S + r'\api_header.h', encoding='utf-8').read())
# keep copies of the two API files next to the generated core so the project is self-contained
import shutil; shutil.copy(S + r'\api_body.cpp', R + r'\engine_api_body.cpp.in'); shutil.copy(S + r'\gen_core.py', R + r'\gen_core.py')
print('generated', len(src[:1432]), 'lines')
