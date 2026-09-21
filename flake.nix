{
  description = "airmonitor - ESP32-S3 environment monitor firmware dev shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";

    flake-parts.url = "github:hercules-ci/flake-parts";

    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    git-hooks-nix = {
      url = "github:cachix/git-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    nixpkgs-esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    inputs@{
      flake-parts,
      treefmt-nix,
      git-hooks-nix,
      nixpkgs-esp-dev,
      ...
    }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" ];

      imports = [
        treefmt-nix.flakeModule
        git-hooks-nix.flakeModule
      ];

      perSystem =
        { config, pkgs, ... }:
        {
          treefmt = {
            projectRootFile = "flake.nix";

            programs = {
              nixfmt.enable = true;

              "clang-format" = {
                enable = true;
                package = pkgs.clang-tools;
              };
            };

            settings.excludes = [
              "*.lock"
              ".git/**"
              "build/**"
              "managed_components/**"
            ];
          };

          pre-commit = {
            check.enable = false;

            settings.hooks = {
              treefmt.enable = true;
            };
          };

          devShells.default = pkgs.mkShell {
            name = "airmonitor";

            inputsFrom = [
              nixpkgs-esp-dev.devShells.${pkgs.stdenv.hostPlatform.system}.esp-idf-full
              config.pre-commit.devShell
            ];

            packages = [
              pkgs.clang-tools
              config.treefmt.build.wrapper
            ];
          };
        };
    };
}
