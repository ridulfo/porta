{
  pkgs ? import <nixpkgs> { },
}:
pkgs.stdenv.mkDerivation (finalAttrs: {
  pname = "porta";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    gcc
    gnumake

  ];
  makeFlags = [ "CC=${pkgs.stdenv.cc}/bin/gcc" ];

  installPhase = ''
    mkdir -p $out/bin
    cp build/release/porta $out/bin
  '';
  meta = with pkgs.lib; {
    description = "Dead-simple, ultra-portable word processor";
    homepage = "https://github.com/ridulfo/porta";
    license = licenses.gpl3;
    maintainers = with maintainers; [ ridulfo ];
    platform = platforms.unix;
  };
})
