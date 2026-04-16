{
  stdenv,
  nix-gitignore,
  meson,
  ninja,

  arrow-cpp,
  boost,
  libzip,
  pkg-config,
}:

stdenv.mkDerivation {
  pname = "mzpeak";
  version = "1.0.0";
  src = nix-gitignore.gitignoreRecursiveSource [ ] ../.;

  doCheck = true;

  nativeBuildInputs = [
    meson
    ninja
  ];

  buildInputs = [
    arrow-cpp
    boost
    libzip
    pkg-config
  ];
}
