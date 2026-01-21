{
  description = "Absinthe wayland compositor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
  };

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "absinthe";
        version = "0.1";

        src = ./.;

        nativeBuildInputs = with pkgs; [
          gcc
          gnumake
          pkg-config
          wayland-scanner
        ];

        buildInputs = with pkgs; [
          wayland
          wayland-protocols
          wlroots_0_19
          pixman
          libxkbcommon
        ];

        buildPhase = ''
          make
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp absinthe $out/bin/
        '';
      };

      devShells."${system}".default = pkgs.mkShell {
        packages = with pkgs; [
          pkg-config
          gnumake
          gcc
          wayland
          wayland-protocols
          wayland-scanner
          wlroots_0_19
          pixman
          libxkbcommon
          fish
        ];

        shellHook = ''
          echo "Absinthe dev shell"
          exec fish
        '';
      };
    };
}
