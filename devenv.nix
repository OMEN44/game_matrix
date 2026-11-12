{
  pkgs,
  lib,
  config,
  inputs,
  ...
}:

{
  packages = with pkgs; [
    platformio
    avrdude
    picotool
  ];
}
