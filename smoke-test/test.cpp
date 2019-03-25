/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Author: Ugo Pattacini <ugo.pattacini@iit.it>
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
*/
#include <string>

#include <yarp/robottestingframework/TestCase.h>
#include <robottestingframework/dll/Plugin.h>
#include <robottestingframework/TestAssert.h>


#include <yarp/os/all.h>
#include <iCub/eventdriven/all.h>

using namespace std;
using namespace robottestingframework;

class spikeChecker : public yarp::os::BufferedPort< ev::vBottle >
{

private:
    int x, y, width, height;
    int inlier;
    int outlier;

    void onRead(ev::vBottle &input) {

        ev::vQueue q = input.getAll();
        for(ev::vQueue::iterator qi = q.begin(); qi != q.end(); qi++) {
            ev::event<ev::AddressEvent> v = ev::as_event<ev::AddressEvent>(*qi);
            int vx = v->x;
            int vy = v->y;
            if(vx >= x && vx <= x + width && vy >= y && vy <= y + height)
                inlier++;
            else
                outlier++;
        }

    }

public:

    spikeChecker() : x((304-10)/2), y((240-10)/2), width(10), height(10), inlier(0), outlier(0)
    {
        this->useCallback();
        this->setStrict();
    }

    void setROI(int x, int y, int width, int height)
    {
        this->x = x; this->y = y; this->width = width; this->height = height;
    }

    int getInliers()
    {
        return inlier;
    }

    int getOutliers()
    {
        return outlier;
    }


};


/**********************************************************************/
class TestAssignmentEventSpikingModel : public yarp::robottestingframework::TestCase
{

private:

    spikeChecker spkchk;
    yarp::os::RpcClient playercontroller;


public:
    /******************************************************************/
    TestAssignmentEventSpikingModel() :
        yarp::robottestingframework::TestCase("TestAssignmentEventSpikingModel")
    {
    }

    /******************************************************************/
    virtual ~TestAssignmentEventSpikingModel()
    {
    }

    /******************************************************************/
    virtual bool setup(yarp::os::Property& property)
    {

        //we need to load the data file into yarpdataplayer
        std::string cntlportname = "/playercontroller/rpc";

        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(playercontroller.open(cntlportname),
                                  "Could not open RPC to yarpdataplayer");

        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect(cntlportname, "/yarpdataplayer/rpc:i"),
                                  "Could not connect RPC to yarpdataplayer");

        //we need to check the output of yarpdataplayer is open and input of spiking model
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/zynqGrabber/vBottle:o", "/vSpikingModel/vBottle:i", "udp"),
                                  "Could not connect yarpdataplayer to spiking model");

        //check we can open our spike checking consumer
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(spkchk.open("/spikechecker/vBottle:i"),
                                  "Could not open spike checker");

        //the output of spiking model
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(yarp::os::Network::connect("/vSpikingModel/vBottle:o", "/spikechecker/vBottle:i", "udp"),
                                  "Could not connect spiking model to spike checker");

        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Ports successfully open and connected");

        spkchk.setROI(55, 25, 40, 130);

        return true;
    }

    /******************************************************************/
    virtual void tearDown()
    {
        ROBOTTESTINGFRAMEWORK_TEST_REPORT("Closing Clients");
        playercontroller.close();
    }

    /******************************************************************/
    virtual void run()
    {

        //play the dataset
        yarp::os::Bottle cmd, reply;
        cmd.addString("play");
        playercontroller.write(cmd, reply);
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(reply.get(0).asString() == "ok", "Did not successfully play the dataset");

        yarp::os::Time::delay(5);

        cmd.clear();
        cmd.addString("stop");
        playercontroller.write(cmd, reply);
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(reply.get(0).asString() == "ok", "Did not successfully stop the dataset");

        int inliers = spkchk.getInliers();
        int outliers = spkchk.getOutliers();
        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("Inliers = %d", inliers));
        ROBOTTESTINGFRAMEWORK_TEST_REPORT(Asserter::format("Outliers = %d", outliers));
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(inliers > 7500, "Inlier score too low (5000)");
        ROBOTTESTINGFRAMEWORK_ASSERT_ERROR_IF_FALSE(outliers < 2500, "Outlier score too high (1000)");


    }
};

ROBOTTESTINGFRAMEWORK_PREPARE_PLUGIN(TestAssignmentEventSpikingModel)
