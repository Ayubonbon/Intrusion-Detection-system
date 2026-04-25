{
  description = "Ayusssii Pwoject <3";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        pythonEnv = pkgs.python3.withPackages (
          ps: with ps; [
            pyserial
          ]
        );
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            pkgs.arduino-cli
            pythonEnv
          ];

          shellHook = ''
            # Isolate Arduino CLI directories to the project folder
            export ARDUINO_DIRECTORIES_DATA="$PWD/.arduino/data"
            export ARDUINO_DIRECTORIES_DOWNLOADS="$PWD/.arduino/downloads"
            export ARDUINO_DIRECTORIES_USER="$PWD/.arduino/user"
            export ARDUINO_CONFIG_FILE="$PWD/.arduino/arduino-cli.yaml"

            echo "Checking ESP32 Arduino Core..."

            if [ ! -f "$ARDUINO_CONFIG_FILE" ]; then
              echo "Initializing local arduino-cli configuration..."
              arduino-cli config init --dest-dir "$PWD/.arduino" > /dev/null
              arduino-cli config set board_manager.additional_urls "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"
            fi

            if ! arduino-cli core list | grep -q "esp32:esp32"; then
              echo "Downloading and installing ESP32 core (this may take a minute)..."
              arduino-cli core update-index
              arduino-cli core install esp32:esp32
              echo "ESP32 core installed successfully!"
            else
              echo "ESP32 core is already installed."
            fi

            echo "----------------------------------------------------"
            echo "🛠️ ESP32 Development Environment Ready!"
            echo "----------------------------------------------------"
            echo "Useful commands:"
            echo "  arduino-cli board list                # Show connected boards"
            echo "  arduino-cli compile -b esp32:esp32:esp32  # Compile your sketch"
            echo "  arduino-cli upload -b esp32:esp32:esp32 -p /dev/ttyUSB0  # Upload"
          '';
        };
      }
    );
}
