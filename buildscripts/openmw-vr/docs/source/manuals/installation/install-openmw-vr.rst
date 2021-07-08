=================
Install OpenMW-VR
=================

OpenMW-VR is not yet a part of official OpenMW and must be installed separately.

	.. note::
		OpenMW-VR is currently a development fork separate from OpenMW, and is without
		an official release schedule. You will be installing development builds. Tags,
		for those who know what that is, are intermittent and semi-arbitrary, and no
		official "release" has been made.

	.. note::
		Installing OpenMW (the non-VR version) is not required. If you have not run the
		openmw installer, simply run the wizard included with the OpenMW-VR install.

Windows Binaries
================

If you're not sure what any of the different methods mean, you should probably stick to this one.
Simply download the latest build from the following link:
`https://gitlab.com/madsbuvi/openmw/-/jobs/artifacts/openmw-vr/download?job=Windows_Ninja_Engine_Release <https://gitlab.com/madsbuvi/openmw/-/jobs/artifacts/openmw-vr/download?job=Windows_Ninja_Engine_Release>`_
and extract the archive into a new/empty folder. If you have not run the OpenMW install wizard in the past, run the extracted openmw_wizard.exe. Now you can run the extracted openmw-vr.exe to launch the game.

	.. note::
		There is no need to uninstall previous builds, but extract each build into a **new or empty** folder and **do not** extract two builds to the same folder.

Linux
=====

Linux users will have to build from source.

	.. note::
		Unlike OpenMW, OpenMW-VR has bumped the version requirement of OSG to 3.6.5. On some distros,
		this means you'll have to roll your own OSG instead of using what's in the official repos.

	.. note::
		I do not maintain this fork on linux, other than fixing compilation errors caught by the CI. 
		I depend on others to make PRs for anything specific to Linux. 

From Source
===========

Visit the `Development Environment Setup <https://wiki.openmw.org/index.php?title=Development_Environment_Setup>`_
section of the Wiki for detailed instructions on how to build the engine.

Clone the OpenMW-VR repo at https://gitlab.com/madsbuvi/openmw instead of the regular OpenMW repo, and checkout the branch openmw-vr (this should get checked out by default).
