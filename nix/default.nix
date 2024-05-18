{ stdenv
, lib
, regex ? true
}:
let
  version = with lib; removePrefix "v" (elemAt
    (pipe (readFile ../src/main.c) [
      (splitString "\n")
      (filter (hasPrefix "#define PROG_VERSION"))
      head
      (splitString " ")
      last
      (splitString "\"")
    ]) 1);

in
stdenv.mkDerivation {
  pname = "crt";
  inherit version;
  src = lib.cleanSource ./..;

  buildInputs = [

  ];

  makeFlags = [ "PREFIX=$(out)" "RELEASE=1" ] ++ lib.optionals (!regex) [ "NO_REGEX=1" ];
}
