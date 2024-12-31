{ stdenv
, lib
, meson
, ninja
, pkg-config

, debug ? false
}:
let
  version = with lib; elemAt
    (pipe (readFile ../meson.build) [
      (splitString "\n")
      (filter (hasPrefix "  version : "))
      head
      (splitString " : ")
      last
      (splitString "'")
    ]) 1;
in
stdenv.mkDerivation {
  pname = "crt";
  inherit version;
  src = lib.cleanSource ./..;

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
  ];

  preConfigure = ''
    patchShebangs ./deps/build.sh
    ./deps/build.sh
  '';

  mesonBuildType = if debug then "debug" else "release";

  meta = {
    description = "Commands Run Today";
    mainProgram = "crt";
    homepage = "https://github.com/spitulax/crt";
    license = lib.licenses.mit;
    platforms = lib.platforms.linux;
  };
}
