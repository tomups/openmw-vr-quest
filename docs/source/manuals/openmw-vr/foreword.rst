Foreword
########

How to read the manual
**********************

The manual can be roughly divided into two parts: a tutorial part that will 
introduce you to the VR interface and what the available options are, and
a pitfalls part that will explain (current) known shortcomings of the VR port
and pitfalls of VR in general.

The rest of this page will explain some terminology.

Terminology
***********
A brief explanation of terms and abbreviation
 - VR: Virtual Reality
 - VR Stage: Your physical play area
 - Mirror texture: The motion picture shown on your pancake monitor when you are playing in VR
 - OpenXR: An Open Source interface for accessing AR/VR devices.

    This is the VR equivalent of Vulkan, developed by the same group as Vulkan: Khronos. OpenXR, like Vulkan, is purely an interface and not an implementation.
 - OpenXR Runtime: An implementation of the OpenXR standard.
 - VR Runtime: The software interfacing with your VR device. An OpenXR Runtime is a subset of a VR runtime.
 - Native Runtimes: The VR Runtime provided by your device drivers.
 - SteamVR: Valve's VR Runtime, shipped as a part of Steam. SteamVR, native to the Index, acts also as a non-native runtime to other headsets
