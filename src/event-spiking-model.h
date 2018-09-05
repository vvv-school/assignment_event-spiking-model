#include <yarp/os/all.h>
#include <yarp/sig/Matrix.h>
#include <yarp/sig/Image.h>
#include <iCub/eventdriven/all.h>

using namespace ev;

class spikingModel : public yarp::os::BufferedPort<vBottle>
{
private:

    //parameters
    bool strict;
    double tau;
    double Te;

    //internal storage
    yarp::sig::ImageOf<yarp::sig::PixelFloat> energymap;
    yarp::sig::ImageOf<yarp::sig::PixelInt> timemap;

    //ouput ports
    yarp::os::BufferedPort<vBottle> outputPort;
    yarp::os::BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono> > debugPort;

    //private functions
    void updateModel(int x, int y, int ts, double inj = 0);
    bool spikeAndReset(int x, int y);

public:

    spikingModel() : strict(false), tau(1000000), Te(1) {}
    bool initialise(std::string name, bool strict = false, unsigned int height = 240, unsigned int width = 304, double tau = 1000000, double Te = 1);
    void close();
    void interrupt();

    void onRead(vBottle &input);

};

class spikingConfiguration : public yarp::os::RFModule
{

private:

    spikingModel spikingmodel;

public:

    spikingConfiguration()  {}
    bool configure(yarp::os::ResourceFinder &rf);

    bool interruptModule() {
        spikingmodel.interrupt();
        return yarp::os::RFModule::interruptModule();
    }
    bool close() {
        spikingmodel.close();
        return yarp::os::RFModule::close();
    }
    double getPeriod() { return 1.0; }
    bool updateModule() { return !isStopping(); }

};
