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
          default = self.packages.${system}.mzpeak;

          mzpeak = pkgs.callPackage nix/package.nix {
            # Override clang on macOS:
            stdenv = if pkgs.stdenv.isDarwin then pkgs.overrideCC pkgs.stdenv pkgs.clang_22 else pkgs.stdenv;
          };

          mzpeak_clang = self.packages.${system}.mzpeak.override (_: {
            # Always use clang.
            stdenv = pkgs.overrideCC pkgs.stdenv pkgs.clang_22;
          });
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
