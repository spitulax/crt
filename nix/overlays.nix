{ self, lib, inputs }: {
  default = final: prev: {
    crt = final.callPackage ./default.nix { regex = true; };
  };
}
