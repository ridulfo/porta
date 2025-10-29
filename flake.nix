{
  description = "Dead-simple, ultra-portable word processor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgsFor = system: nixpkgs.legacyPackages.${system};
    in {
      packages = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in {
          default = pkgs.stdenv.mkDerivation {
            pname = "porta";
            version = "0.1.0";

            src = ./.;

            nativeBuildInputs = with pkgs; [
              gcc
              gnumake
            ];

            makeFlags = [ "CC=${pkgs.stdenv.cc}/bin/gcc" ];

            installPhase = ''
              runHook preInstall
              mkdir -p $out/bin
              cp build/release/porta $out/bin
              runHook postInstall
            '';

            meta = with pkgs.lib; {
              description = "Dead-simple, ultra-portable word processor";
              homepage = "https://github.com/ridulfo/porta";
              license = licenses.gpl3;
              platforms = platforms.unix;
              mainProgram = "porta";
            };
          };
        });

      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
        in {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              gcc
              gnumake
            ];
          };
        });
    };
}
