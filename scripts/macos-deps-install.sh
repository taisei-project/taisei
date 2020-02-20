#!/bin/bash

# This script installs all of the necessary dependencies needed for Taisei
# on macOS.

green=`tput setaf 2`
white=`tput setaf 7`
reset=`tput sgr0`

echo -e "Taisei Project macOS Dev Environment Setup"

# Install Xcode Command Line Utils
if ! xcode-select -p 1>/dev/null
then
    xcode-select --install
    echo -e "${white}Installing Xcode Command Line Utils... ${green}done!${reset}"
else
    echo -e "${white}Installing Xcode Command Line Utils... ${green}done!${white} (already installed)${reset}"
fi

# Install Homebrew

if brew --version | grep -q 'Homebrew' 2>/dev/null
then
    echo -en "${white}Installing Homebrew... ${reset}"
    echo -e "${green}done!${white} (already installed)${reset}"
else
    echo -e " "
    echo -e "Please note that this asks for your root password"
    echo -e "If you are uncomfortable with that, press Ctrl+C and run the command manually:"
    echo -e "/usr/bin/ruby -e \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)\""
    echo -e " "
    read -p "Press enter to continue"

    /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

    echo -en "${white}Installing Homebrew... ${reset}"

    if brew --version | grep -q 'Homebrew'; then
        echo -e "${green}done!${reset}"
    else
        echo -e "${red}failed?!${reset}"
        echo -e "Please investigate while Homebrew failed to install before proceeding!"
        exit 1
    fi

fi

echo -en "${white}Installing dev tools... ${reset}"
brew install meson cmake pkg-config docutils python3 pygments 2>/dev/null
echo -e "${green}done!${reset}"

echo -en "${white}Installing optional dependencies... ${reset}"
brew install freetype libzip opusfile libvorbis webp sdl2 2>/dev/null
echo -e "${green}done!${reset}"


if ! grep -Eq 'export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig"' ~/.zshrc &&
   ! grep -Eq 'export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig"' ~/.bash_profile
then
    echo
    echo -e "${white}Do you want me to append the following to your shell .rc file?${reset}"
    echo -e "   export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig""

    read -p "[Y/N]: " -n 1 -r
    echo -e " "

    if [[ $REPLY =~ ^[Yy]$ ]]
    then
        if [ -n "`$SHELL -c 'echo $ZSH_VERSION'`" ]; then
            echo 'export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig"' >> ~/.zshrc
            echo -e 'Exported PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig to ~/.zshrc'
            echo -e "Please run \`source ~/.zshrc\` to activate"
        elif [ -n "`$SHELL -c 'echo $BASH_VERSION'`" ]; then
            echo 'export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig"' >> ~/.bash_profile
            echo -e 'Exported PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig to ~/.bash_profile'
            echo -e "Please run \`source ~/.bash_profile\` to activate"
        fi
    fi
fi

echo
echo -e "Setup complete!"
