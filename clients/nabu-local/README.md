# NABU Local Wrapper

## Purpose

This is an add-only local NABU wrapper project inside the FujiNet lobby client
tree.

It exists so the NABU lobby can live beside the upstream multi-platform source
without modifying the upstream `clients/Makefile` or shared client files.

## What it does

- builds the NABU local client from `../src/nabu`
- keeps all upstream files untouched
- uses the local older `z88dk` + `NABULIB` setup

## Build paths

The local batch build copies files to:

```txt
NLOBBY.com  -> D:\NIA\NABU Internet Adapter\Store\D\0
NLOBBY.CFG  -> D:\NIA\NABU Internet Adapter\Store
```

After a normal build, the local output folder is trimmed back so only
`NLOBBY.com` is left in `r2r\nabu_cpm`, and the generated `.lis` files are
removed from `../src/nabu`.

## Notes

- `Makefile` is provided for structure and future compatibility work
- `build-nabu.bat` is the reliable local build entry point right now
- `build-nabu.bat clean` removes the local generated output files in `r2r\nabu_cpm` and the generated `.lis` files in `../src/nabu`
- the local NABU client is built as a single translation unit because
  `NABU-LIB.h` and `RetroNET-FileStore.h` pull in implementation bodies
- this stage does not modify the upstream FujiNet lobby sources

## Current upstream flow note

Study of `fujinet-lobby` shows the following path:

The exact sequence is:

- User presses the trigger or enter key, which calls `mount()`
- `mount()` takes the selected `client_url` and strips off the protocol prefix
- It looks for `://` and skips past it
- The code comment says `assume TNFS://`
- It splits the remainder into host and filename or path
- It checks FujiNet host slots to see whether that host is already present
- If yes, it reuses the slot
- If not, it writes the host into the last slot
- It mounts that host slot in FujiNet
- It assigns the selected file to FujiNet device slot `0` and mounts it as a disk image
- It writes the selected game server URL, not the client URL, into the game's AppKey so the launched game knows which server to contact
- Helper implementation: `fujinet-lobby/clients/src/io.c`
- Finally, it reboots or runs the mounted game

This is the tricky bit and one of the main reasons the NABU version needs its own data file and launch path.

## Current limitations

- The NABU Internet Adapter path used here is TCP-based
- Not fully commented at this time
- Some TNFS hosts appear to require a different transport path (UDP) and may not work from this NABU client
- Game launch is still a NABU-specific data file path, not the same mount-and-boot flow used by FujiNet
