{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    { self, nixpkgs, ... }:
    let
      # List of supported systems:
      supportedSystems = nixpkgs.lib.platforms.unix;

      # Iterate over each system:
      each =
        f:
        nixpkgs.lib.genAttrs supportedSystems (
          system:
          let
            pkgs = import nixpkgs { inherit system; };
          in
          f pkgs system
        );
    in
    {
      packages = each (
        pkgs: system: {
          default = pkgs.callPackage nix/package.nix { };
        }
      );

      checks = each (pkgs: system: self.packages.${system});

      devShells = each (
        pkgs: system: {
          default = pkgs.mkShell {
            name = "mzpeak shell";
            hardeningDisable = [ "fortify" ];
            inputsFrom = [ self.packages.${system}.default ];
            buildInputs = [
              pkgs.clang-tools
              pkgs.python3
              pkgs.ruff
            ];
          };
        }
      );
    };
}
