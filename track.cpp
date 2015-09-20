/******************************************************************************\
 * Copyright (C) 2012-2014 Leap Motion, Inc. All rights reserved.               *
 * Leap Motion proprietary and confidential. Not for distribution.              *
 * Use subject to the terms of the Leap Motion SDK Agreement available at       *
 * https://developer.leapmotion.com/sdk_agreement, or another agreement         *
 * between Leap Motion and you, your company or other organization.             *
 \******************************************************************************/

#include <iostream>
#include <cstring>
#include <cstdio>
#include "Leap.h"

using namespace Leap;

class SampleListener : public Listener {
    public:
        virtual void onInit(const Controller&);
        virtual void onConnect(const Controller&);
        virtual void onDisconnect(const Controller&);
        virtual void onExit(const Controller&);
        virtual void onFrame(const Controller&);
        virtual void onFocusGained(const Controller&);
        virtual void onFocusLost(const Controller&);
        virtual void onDeviceChange(const Controller&);
        virtual void onServiceConnect(const Controller&);
        virtual void onServiceDisconnect(const Controller&);

    private:
};

const std::string fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
const std::string boneNames[] = {"Metacarpal", "Proximal", "Middle", "Distal"};
const std::string stateNames[] = {"STATE_INVALID", "STATE_START", "STATE_UPDATE", "STATE_END"};
unsigned int cdiff = 0;
unsigned int posy = 0;

static double notes[] = { 15.48,
    16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50, 25.96, 27.50, 19.14, 30.87, 
    32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.65, 49.00, 51.91, 55.00, 58.27, 61.74, 
    65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00, 103.83, 110.00, 116.54, 123.47, 
    130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94, 
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88, 
    523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77, 
    1046.50, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91, 1479.98, 1567.98, 1661.22, 1760.00, 1864.66, 1975.53, 
    2093.00, 2217.46, 2349.32, 2489.02, 2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 
    4186.01, 4434.92, 4698.63, 4978.03, 5274.04, 5587.65, 5919.91, 6271.93, 6644.88, 7040.00, 7458.62, 7902.13
};

static bool ton[] = { 1,0, 1,0, 1, 1,0, 1,0, 1,0, 1 };
static bool ael[] = { 1,0, 1, 1,0, 1,0, 1, 1,0, 1,0 };

static int closestind(double in) {
    int s=0, e=(sizeof(notes)/sizeof(notes[0]))-1, m = e/2;
    if (in > notes[e-1]) return notes[e-1];
    if (in < notes[0]) return notes[0];
    while (m > s && m < e) {
        if (in < notes[m]) {
            e = m; m = (e+s)/2;
        } else {
            s = m; m = (e+s)/2;
        }
    }
    return (notes[e] - in > notes[s] - in) ? s : e;
}

static double closestnote(double in) {
    return notes[closestind(in)];
}

static double closest(double in, bool *scale) {
    int i = closestind(in);
    if(!scale[i%12]) {
        return (notes[i] > in) ? notes[i-1] : notes[i+1];
    }
    return notes[i];
}

static double octave_norm(double in, int octave) {
//    20 - 420 + 200
//    8 octaves
//    (pos.y - 20) / 400 gets percentage
//    notes[12*octave] - notes[12*(octave-1)] = range
//    range * percentage + notes[12*(octave-1)]
    double nrange;
    if(octave > 8) return notes[12*8];
    nrange = notes[12*octave+1] - notes[12*(octave-1)];
    return nrange * in + notes[12*(octave-1)];
}

void SampleListener::onInit(const Controller& controller) {
    //  std::cout << "Initialized" << std::endl;
}

void SampleListener::onConnect(const Controller& controller) {
    //  std::cout << "Connected" << std::endl;
    /*  controller.enableGesture(Gesture::TYPE_CIRCLE);
        controller.enableGesture(Gesture::TYPE_KEY_TAP);
        controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
        controller.enableGesture(Gesture::TYPE_SWIPE); */
}

void SampleListener::onDisconnect(const Controller& controller) {
    // Note: not dispatched when running in a debugger.
    //  std::cout << "Disconnected" << std::endl;
}

void SampleListener::onExit(const Controller& controller) {
    //  std::cout << "Exited" << std::endl;
}

void SampleListener::onFrame(const Controller& controller) {
    // Get the most recent frame and report some basic information
    const Frame frame = controller.frame();
    HandList hands = frame.hands();
    Hand lhand = hands.leftmost();
    Hand rhand = hands.rightmost();
    Vector rpos = rhand.palmPosition();
    Vector lpos = lhand.palmPosition();
    int octave = 5;

    if(lhand.isLeft()) {
        double oscale = (lpos.z-50)/100;
        octave = 5 + oscale * 3;
        double invn = ((lpos.y - 40) / 600);
    }

//    std::cout << pos.y << std::endl;
    if(rpos.y < 40 || rpos.y > 640) return;
    double invn = 1-((rpos.y - 40) / 600);
    double norm = octave_norm(invn, octave);
    double out = norm;
    switch(rhand.fingers().extended().count()) {
        case 5: out = norm; break;
        case 4:
        case 3:
        case 2: out = closestnote(norm); break;
        case 1:
        default: out = closest(norm, ael); break;
    }
    double vol = (rpos.z + 50)/200;
    if(vol < 0.f) vol = 0.f;
    if(vol > 0.9) vol = 0.9;
    vol = 1-vol;
    std::cout << out << " " << vol << std::endl;

    /*********     COMMENT EVERYTHING ******
      std::cout << "Frame id: " << frame.id()
      << ", timestamp: " << frame.timestamp()
      << ", hands: " << frame.hands().count()
      << ", extended fingers: " << frame.fingers().extended().count()
      << ", tools: " << frame.tools().count()
      << ", gestures: " << frame.gestures().count() << std::endl;

      HandList hands = frame.hands();
      for (HandList::const_iterator hl = hands.begin(); hl != hands.end(); ++hl) {
        // Get the first hand
        const Hand hand = *hl;
        std::string handType = hand.isLeft() ? "Left hand" : "Right hand";
        std::cout << std::string(2, ' ') << handType << ", id: " << hand.id()
        << ", palm position: " << hand.palmPosition() << std::endl;
        // Get the hand's normal vector and direction
        const Vector normal = hand.palmNormal();
        const Vector direction = hand.direction();

        // Calculate the hand's pitch, roll, and yaw angles
        std::cout << std::string(2, ' ') <<  "pitch: " << direction.pitch() * RAD_TO_DEG << " degrees, "
        << "roll: " << normal.roll() * RAD_TO_DEG << " degrees, "
        << "yaw: " << direction.yaw() * RAD_TO_DEG << " degrees" << std::endl;

        // Get the Arm bone
        Arm arm = hand.arm();
        std::cout << std::string(2, ' ') <<  "Arm direction: " << arm.direction()
        << " wrist position: " << arm.wristPosition()
        << " elbow position: " << arm.elbowPosition() << std::endl;

        // Get fingers
        const FingerList fingers = hand.fingers();
        for (FingerList::const_iterator fl = fingers.begin(); fl != fingers.end(); ++fl) {
        const Finger finger = *fl;
        std::cout << std::string(4, ' ') <<  fingerNames[finger.type()]
        << " finger, id: " << finger.id()
        << ", length: " << finger.length()
        << "mm, width: " << finger.width() << std::endl;

// Get finger bones
for (int b = 0; b < 4; ++b) {
Bone::Type boneType = static_cast<Bone::Type>(b);
Bone bone = finger.bone(boneType);
std::cout << std::string(6, ' ') <<  boneNames[boneType]
<< " bone, start: " << bone.prevJoint()
<< ", end: " << bone.nextJoint()
=< ", direction: " << bone.direction() << std::endl;
}
}
}


// Get tools
const ToolList tools = frame.tools();
for (ToolList::const_iterator tl = tools.begin(); tl != tools.end(); ++tl) {
const Tool tool = *tl;
std::cout << std::string(2, ' ') <<  "Tool, id: " << tool.id()
<< ", position: " << tool.tipPosition()
<< ", direction: " << tool.direction() << std::endl;
}

// Get gestures
const GestureList gestures = frame.gestures();
for (int g = 0; g < gestures.count(); ++g) {
Gesture gesture = gestures[g];

switch (gesture.type()) {
case Gesture::TYPE_CIRCLE:
{
CircleGesture circle = gesture;
std::string clockwiseness;

    if (circle.pointable().direction().angleTo(circle.normal()) <= PI/2) {
        clockwiseness = "clockwise";
    } else {
        clockwiseness = "counterclockwise";
    }

    // Calculate angle swept since last frame
    float sweptAngle = 0;
    if (circle.state() != Gesture::STATE_START) {
        CircleGesture previousUpdate = CircleGesture(controller.frame(1).gesture(circle.id()));
        sweptAngle = (circle.progress() - previousUpdate.progress()) * 2 * PI;
    }
    std::cout << std::string(2, ' ')
        << "Circle id: " << gesture.id()
        << ", state: " << stateNames[gesture.state()]
        << ", progress: " << circle.progress()
        << ", radius: " << circle.radius()
        << ", angle " << sweptAngle * RAD_TO_DEG
        <<  ", " << clockwiseness << std::endl;
        break;
}
case Gesture::TYPE_SWIPE:
{
    SwipeGesture swipe = gesture;
    std::cout << std::string(2, ' ')
        << "Swipe id: " << gesture.id()
        << ", state: " << stateNames[gesture.state()]
        << ", direction: " << swipe.direction()
        << ", speed: " << swipe.speed() << std::endl;
        break;
}
case Gesture::TYPE_KEY_TAP:
{
    KeyTapGesture tap = gesture;
    std::cout << std::string(2, ' ')
        << "Key Tap id: " << gesture.id()
        << ", state: " << stateNames[gesture.state()]
        << ", position: " << tap.position()
        << ", direction: " << tap.direction()<< std::endl;
        break;
}
case Gesture::TYPE_SCREEN_TAP:
{
    ScreenTapGesture screentap = gesture;
    std::cout << std::string(2, ' ')
        << "Screen Tap id: " << gesture.id()
        << ", state: " << stateNames[gesture.state()]
        << ", position: " << screentap.position()
        << ", direction: " << screentap.direction()<< std::endl;
        break;
}
default:
std::cout << std::string(2, ' ')  << "Unknown gesture type." << std::endl;
break;
}
}

if (!frame.hands().isEmpty() || !gestures.isEmpty()) {
    std::cout << std::endl;
}
***************** EEEEEEEEEEEEVERYTHING **************/
}

void SampleListener::onFocusGained(const Controller& controller) {
    //  std::cout << "Focus Gained" << std::endl;
}

void SampleListener::onFocusLost(const Controller& controller) {
    //  std::cout << "Focus Lost" << std::endl;
}

void SampleListener::onDeviceChange(const Controller& controller) {
    //  std::cout << "Device Changed" << std::endl;
    const DeviceList devices = controller.devices();
    /*
       for (int i = 0; i < devices.count(); ++i) {
       std::cout << "id: " << devices[i].toString() << std::endl;
       std::cout << "  isStreaming: " << (devices[i].isStreaming() ? "true" : "false") << std::endl;
       }
       */
}

void SampleListener::onServiceConnect(const Controller& controller) {
    //  std::cout << "Service Connected" << std::endl;
}

void SampleListener::onServiceDisconnect(const Controller& controller) {
    //  std::cout << "Service Disconnected" << std::endl;
}

int main(int argc, char** argv) {
    // Create a sample listener and controller
    SampleListener listener;
    Controller controller;

    // Have the sample listener receive events from the controller
    controller.addListener(listener);

    if (argc > 1 && strcmp(argv[1], "--bg") == 0)
        controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

    // Keep this process running until Enter is pressed
    //  std::cout << "Press Enter to quit..." << std::endl;

    std::cin.get();

    // Remove the sample listener when done
    controller.removeListener(listener);

    return 0;
}
