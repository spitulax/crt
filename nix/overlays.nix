{ self, lib, inputs }: {
  default = final: prev: rec {
    crt = final.callPackage ./default.nix { };
    crt-debug = crt.override { debug = true; };
  };
}
