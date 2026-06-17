# BokutachiIR_OpenLR2

OpenLR2 CustomIR module made for [Bokutachi IR](https://boku.tachi.ac/).

For the module to work _BokutachiAuth.json_ file must be present in the same folder as the .dll file.
It contains server URL and your API key.
The file can be generated at <https://boku.tachi.ac/client-file-flow/CXLR2Hook>.

Put the .dll into _LR2files/CustomIRs/BokutachiIR_.
You must create those folders yourself.
In the end, you will have the files placed in the following fashion:

```
LR2files/CustomIRs/BokutachiIR/
  - BokutachiAuth.json
  - BokutachiIR.x64.dll (or BokutachiIR.x86.dll)
  - BokutachiIR.x64.pdb (or BokutachiIR.x86.pdb) (optional, if you are a developer)
```

_BokutachiAuth.json_ will look like this:

```json
{
  "url": "https://boku.tachi.ac/ir/lr2hook/import",
  "apiKey": "afafafffafafafffafafafffafafafffafafafaf"
}
```

## Building

See GitHub actions workflow files for an example.
