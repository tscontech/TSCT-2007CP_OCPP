Build environment for Linux.

To build projects on Ubuntu, you need

1. Install following package on Ubuntu.

   sudo apt-get install cmake
   sudo apt-get install qtbase5-dev

   sudo apt-get install git p7zip-full (option)

Notes: If you do not need the graphical configuration interface, and just need
to build code on the Linux environment. It only requires following package to
build it.

   sudo apt-get install cmake libc6-i386

2. install toolchains to /opt/ITEGCC

3. Run projects on build/linux/*.sh

   For example, run build/linux/doorbell_indoor_al.sh to indoor project.

   TO autobuild the project,

   export AUTOBUILD=1
   build/linux/doorbell_indoor_all.sh


PS: It's fully test on Ubuntu 14.04 64-bit linux version. For othres Linux
distribution, you need install proper shared libraries.


Others Linux distributions build guide.

Ubuntu Linux
Ubuntu 12.04.5
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update
    sudo apt-get install git vim p7zip-full gcc-5-multilib g++-5-multilib
    replace libstdc++.so.6

Ubuntu 14.04.6
    sudo apt-get install qtbase5-dev git vim p7zip-full

Ubuntu 16.04.6
    sudo apt-get install qtbase5-dev git vim p7zip-full make

Ubuntu 18.04.4
    sudo apt-get install qtbase5-dev git vim p7zip-full make

Ubuntu 20.04
    Normal Installation
    sudo apt-get install qtbase5-dev git vim p7zip-full make libncurses5


OpenSUSE Linux
openSUSE 12.3
    sudo zypper install git libncurses5 make
    replace libstdc++.so.6

openSUSE 13.2
    sudo zypper install git libncurses5

openSUSE 42.3
    sudo zypper install git libncurses5

openSUSE 15.1
    sudo zypper install git libncurses5 


CentOS Linux
CentOS 7 1905
    KDE Plasma Workspaces Installation: Development Tools
    nothing
    
CentOS 8 1911
    GNOME Application, Development Tools
    sudo yum install epel-release
    sudo yum install p7zip ncurses-compat-libs.x86_64 (for mconf)
 
    
Debian Linux    
Debian 7.11.0: Debian desktop environment, standard system utilities
    sudo apt-get install git vim make libc6-dev (not support qt5)

Debian 8.11.1: Debian desktop environment, KDE, standard system utilities
    sudo apt-get install qtbase5-dev git vim make

Debian 9.11.0: Debian desktop environment, KDE, standard system utilities
    sudo apt-get install git vim make

Debian 10.4.0: Debian desktop environment, KDE, standard system utilities
    sudo apt-get install git vim make libncurses5


Windows Subsystem for Linux 1, only Windows 10 Pro 1809/Enterprise or above.
	1. Run Powershell as Administrator
		dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
		
	2. Reboot Windows 10.

	3. Downloading Linux distros, manually download and install.
		Ubuntu 20.04: https://aka.ms/wslubuntu2004
		Ubuntu 18.04: https://aka.ms/wsl-ubuntu-1804
		Ubuntu 16.04: https://aka.ms/wsl-ubuntu-1604
		OpenSUSE Leap 42: https://aka.ms/wsl-opensuse-42
		SUSE Linux Enterprise Server 12: https://aka.ms/wsl-sles-12
		
	4. Installing Linux distro, run Powershell as Administrator
		Add-AppxPackage SLES-12_v1.appx

	5. Run Linux distro from start menu, create username and password.
	
	6. Upgrade Linux distro, for example:
		zypper dup
	
	7. Install some libs or tools, for openSUSE 42.x or SLES-12
		sudo zypper install git libncurses5 p7zip-full make 	