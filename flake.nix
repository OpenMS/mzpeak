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

          # Always use clang.
          mzpeak_clang = self.packages.${system}.mzpeak.override (_: {
            stdenv = pkgs.overrideCC pkgs.stdenv pkgs.clang_22;
          });

          # Build dependencies with debugging symbols.
          mzpeak_debug = self.packages.${system}.mzpeak.override (_: {
            arrow-cpp = pkgs.arrow-cpp.overrideAttrs (orig: {
              cmakeBuildType = "RelWithDebInfo";
              dontStrip = true;
              hardeningDisable = (orig.hardeningDisable or [ ]) ++ [ "fortify" ];
            });
          });
        }
      );

      checks = each (pkgs: system: self.packages.${system});

      devShells = each (
        pkgs: system:
        let
          shell =
            mzpeak:
            pkgs.mkShell {
              name = "mzpeak shell";
              hardeningDisable = [ "fortify" ];
              inputsFrom = [ mzpeak ];
              buildInputs = [
                pkgs.clang-tools
                pkgs.python3
                pkgs.ruff
              ];
            };
        in
        {
          default = shell self.packages.${system}.default;
          debug = shell self.packages.${system}.mzpeak_debug;
        }
      );
    };
}
