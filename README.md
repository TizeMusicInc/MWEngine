MWEngine is..
=============

...an audio engine for Android, using either OpenSL (compatible with Android 4.1 and up) or AAudio
(Android 8.0 and up) as the drivers for low latency audio performance. The engine has been written for both
[MikroWave](https://play.google.com/store/apps/details?id=nl.igorski.mikrowave.free&hl=en) and
[Kosm](https://play.google.com/store/apps/details?id=nl.igorski.kosm&hl=en) to provide fast live audio synthesis. MWEngine is also used by [TIZE - Beat Maker, Music Maker](https://play.google.com/store/apps/details?id=com.tizemusic.tize).

MWEngine provides an architecture that allows you to work with audio within a _musical context_. It is easy to
build upon the base classes and create your own noise generating mayhem. A few keywords describing the
out-of-the-box possibilities are:

 * tempo-based sequencing with support for alternative time signatures
 * on-the-fly audio synthesis
 * multi-channel audio output
 * effect chains operating on individual input/output channels
 * sample playback with real time pitch shifting
 * bouncing output to WAV files, either live (during a performance) or "offline"
 
Also note that MWEngine's underlying audio drivers are _the same as Google Oboe uses_, MWEngine and
Oboe are merely abstraction layers to solve the same problem, only in different ways. Additionally, MWEngine provides a complete audio processing environment.

### The [Issue Tracker](https://github.com/igorski/MWEngine/issues?q=is%3Aissue+is%3Aopen+sort%3Aupdated-desc) is your point of contact

Bug reports, feature requests, questions and discussions are welcome on the GitHub Issue Tracker, please do not send e-mails through the development website. However, please search before posting to avoid duplicates, and limit to one issue per post.

Please vote on feature requests by using the Thumbs Up/Down reaction on the first post.

### C++ ??? What about Java ?

Though the library is written in C++ (and can be used solely within this context), the library can be built using JNI
(Java Native Interface) which makes its API expose itself to Java, while still executing in a native layer outside of
the Dalvik/ART VM. In other words : high performance of the engine is ensured by the native layer operations, while
ease of development is ensured by delegating application logic / UI to the realm of the Android Java SDK.

Whether you intend to use MWEngine for its sample based playback or to leverage its built-in synthesizer and
audio processing, you are not required to write any additional C++ code. If you however intend to create your own
DSP or synthesis routines (which is fun to do!) you must write them in C++, but can rely on SWIG for making them usable in Java.

#### A note on garbage collection and SWIG

It is important to note that when a Java object finalizes (i.e. all its references are broken and is garbage collected), the
destructors of the native objects are invoked, which can lead to unpredictable results if you happen to overlook this!
As such, audio engine objects such as effects processors or events that are created on the Java side, must also hold
strong references during their lifecycle. Basically, follow the same principles you'd use in Java, but be
aware that ignoring these will have a particularly violent result with a very unfriendly stack trace.

### Environment setup

If you intend to use [Android Studio](https://developer.android.com/studio/) you can open the
_build.gradle_ file to setup the project and its dependencies accordingly by following the on-screen
steps.

For CLI builds:

You will need both the [Android SDK](https://developer.android.com/studio/index.html) and the [Android NDK](https://developer.android.com/ndk/downloads/index.html).
Additionally, you will need [SWIG](http://www.swig.org) (available on most package managers like _Brew_ for OS X or _apt-get_ on Linux)

You will need [Gradle](https://gradle.org) to run the build scripts. All aforementioned utilities are available on all major Operating Systems.

### Build instructions

#### Using Android Studio

If you are using Android Studio, creating a project from the supplied _build.gradle_ file should
suffice to get you to build both the native and Java code as well as enabling building, debugging and
packaging directly both from its IDE.

When NDK builds fail, be aware that _ndk-build_ doesn't like it when paths contain spaces (yeah...)

In case the Java build fails due to missing classes, see below to compile the
audio engine as a library using CLI.

#### Using CLI

After making sure you have all the correct tools (see _Environment setup_):

##### Compiling the audio engine as a library

The makefile (_/src/main/cpp/Android.mk_) will by default compile the library with all available modules. The SWIG interface file
(_/src/main/cpp/mwengine.i_) includes all the engine's actors that will be exposed to Java.

Those of a Unix-bent can run the _build.sh_-file in the root folder of the repository whereas Windows users can run the
_build.bat_-file that resides in the same directory, just make sure "_ndk-build_" and "_swig_" are globally available
through the PATH settings of your system (or adjust the shell scripts accordingly).

After compiling the C++ code, the SWIG wrappers will generate the _nl.igorski.lib.audio.mwengine_-namespace, making the code available to Java.

##### Compiling and installing the example Activity

You can create the .APK package and deploy it instantly onto an attached device / emulator by using Gradle e.g. :

    gradle installDebug

To create a signed release build, add the following into the Gradle's properties file inside your
home folder (_~/.gradle/gradle.properties_) and replace the values accordingly:

    RELEASE_STORE_FILE={path_to_.keystore_file}
    RELEASE_STORE_PASSWORD={password_for_.keystore}
    RELEASE_KEY_ALIAS={alias_for_.keystore}
    RELEASE_KEY_PASSWORD={password_for_.keystore}

You can now build and sign a releasable APK by running:

    gradle build

### FAQ / Troubleshooting

The contents of this repository should result in a stable application. If you experience issues with
the setup, consult the [Troubleshooting Wiki page](https://github.com/igorski/MWEngine/wiki/Troubleshooting-MWEngine).

### Documentation

You can view the Wiki (which documents all of the engine's actors as well as a variety of real world
use cases) here:

[https://github.com/igorski/MWEngine/wiki](https://github.com/igorski/MWEngine/wiki)

Note that you can also view the contents of the header files to get more details about the inner
workings of each class.

### Unit tests

The library comes with unit tests (_/src/main/cpp/tests/_), written using the Googletest C++ testing framework (distributed with NDK 10).
To run the tests, simply execute the _test.sh_ (sorry Unix-only shell at the moment)-script with a device attached.
This will also build the library prior to running the tests by calling the build script described above.
Note: _adb_ must be available in your global path settings.

*Note on unit testing:* To build the application for unit testing observe that there is a separate makefile for the
unit test mode (see _Application_test.mk_). In short: this file sets the compiler preprocesser MOCK_ENGINE which
replaces the OpenSL driver with a mocked driver so the engine can be unit tested "offline".

### Demo

The repository contains an example Activity that is ready to deploy onto any Android device/emulator supporting ARM-, ARMv7-,
x86- architecture and running Android 4.1 or higher. The example will demonstrate how to quickly get a musical
sequence going using the library.

To install the demo: first build the library as described above, and then run the build script to deploy the .APK onto an
attached device/emulator (note that older emulated devices can only operate at a sample rate of 8 kHz!).

### Note on AAudio

The AAudio implementation has been built using (in Google's words): _"a Preview release of the AAudio library. The API
might change in backward-incompatible ways in future releases. It is not recommended for production use."_ so use it
at your own peril. To use AAudio instead of OpenSL:
 
 * change the desired driver in _global.h_ from type 0 (OpenSL) to 1 (AAudio)
 * update the _Android.mk_ file to include all required adapters and libraries (simply set _BUILD_AAUDIO_ to 'true')
 * update target in _project.properties_ to _android-26_
 
Once AAudio is a stable library, MWEngine will allow on-the-fly switching between OpenSL and AAudio drivers.

(!) MWEngine does not support recording from the device inputs using AAudio just yet, (https://github.com/igorski/MWEngine/issues/70) references this feature.

### Contributors

MWEngine has received welcome contributions (either suggestions on improving the API or proposal of new features,
solving of bugs, etc.) from the following developers :

 * Andrey Stavrinov (@hypeastrum)
 * Toufik Zitouni & Robert Avellar (Tize)
 * Koert Gaaikema (@koertgaaikema)
 * Matt Logan (@mattlogan)
 * Thomas Flasche (@harthorst)
 * Rickard Östergård (@rckrdstrgrd)
 * Aran Arunakiri
