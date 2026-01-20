{
  description = "Absinthe wayland compositor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64_linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
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
        ];
      };
    };
}
