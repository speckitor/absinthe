{
  description = "Absinthe wayland compositor";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
  };

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
    in
    {
      devShells."${system}".default =
      let
        pkgs = import nixpkgs { inherit system; };
      in
      pkgs.mkShell {
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
          zsh
        ];

        shellHook = ''
          echo "Absinthe dev shell"
          exec zsh
        '';
      };
    };
}
