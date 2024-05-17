{ stdenv
, lib
, regex ? true
}:
stdenv.mkDerivation {
  pname = "crt";
  version = "0.1.0";
  src = lib.cleanSource ./..;

  buildInputs = [

  ];

  makeFlags = [ "PREFIX=$(out)" "RELEASE=1" ] ++ lib.optionals (!regex) [ "NO_REGEX=1" ];
}
