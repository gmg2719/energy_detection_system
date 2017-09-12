#include <uhd/types/tune_request.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>
#include <thread>
#include <gnuplot-iostream.h>


#define TC_ADDRESS 0
#define TH_ADDRESS 1
#define RESET_ADDRESS 2
#define WINDOW_SIZE_ADDRESS 3


namespace po = boost::program_options;

class ed_type{
public:
  uint32_t detect;
  uint32_t sq_m;
};

uint32_t value_terminal_count(double sample_rate){

  uint32_t terminal_count;

  terminal_count= (100e6/sample_rate)-1;

  return terminal_count;

}


int UHD_SAFE_MAIN(int argc, char *argv[]){

    uhd::set_thread_priority_safe();

    std::string args("addr=192.168.192.1,type=usrp2");//multi uhd device address args
    std::string file("Freauency Span 0");//name of the file to write binary samples
    std::string type("sc16");//sample type: double, float, or short
    std::string ant("RX2");
    std::string subdev("A:0");
    std::string ref("internal");
    std::string wirefmt("sc16");
    size_t total_num_samps(2048);//total number of samples to receive
    size_t spb(4096);// samples per buffer
    double rate(10.0e6);// rate incoming samples
    double freq(390.0e6);//RF center Frequency
    double freq_span(10.0e6);
    double gain(0);//gain of the amplifier
    double total_time(0.0);//total number of seconds to receive
    double setup_time(1.0);
    int window_size;
    int threshold;
    int N(1);
    int refresh_graph(200);
    double span;
    double delta;
    

    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
      ("help", "help message")
      ("rate", po::value<double>(&rate)->default_value(10e6), "sample rate")
      ("file", po::value<std::string>(&file), "Name of the Graph")
      ("freq", po::value<double>(&freq)->default_value(100e6), "RF center frequency in Hz")
      ("gain", po::value<double>(&gain)->default_value(0), "Gain for the RF chain")
      ("th",po::value<int>(&threshold)->default_value(14),"Threshold integer value")
      ("wsz",po::value<int>(&window_size)->default_value(8),"Window size value only power of 2 greater than 8")
      ("n",po::value<int>(&N)->default_value(1),"Number of frequency span starting from the central frequency")
      ("span",po::value<double>(&span)->default_value(0.0035),"Scan time sec")
      ("delta",po::value<double>(&delta)->default_value(0.002),"Delta time sec")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD RX samples to file %s") % desc << std::endl;
        std::cout
            << std::endl
            << "This application receives data from a single channel of a USRP device changing the frequency and analysing the spectrum, plot the result usign GNU Plot. Can work only with the fixed threshold fpga image\n"
            << std::endl;
        return 0;
    }

    //create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);//instantiation of the usrp variable

    //Lock mboard clocks
    usrp->set_clock_source(ref);

    //always select the subdevice first, the channel mapping affects the other settings
    usrp->set_rx_subdev_spec(subdev);
    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    //set the sample rate
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate/1e6) << std::endl;
    usrp->set_rx_rate(rate);
    std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate()/1e6) << std::endl << std::endl;

    // set the terminal count of the FFT_FPGA_image_v2
    uint32_t tc = value_terminal_count(rate);
    usrp->set_user_register(TC_ADDRESS,tc,0);
    std::cout << boost::format("Value for the terminal count selected is: %d")%tc << std::endl << std::endl;

    //set the rf gain
    std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
    usrp->set_rx_gain(gain);
    std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain() << std::endl << std::endl;

    //set the antenna
    usrp->set_rx_antenna(ant);
    std::cout << boost::format("Name sensor ")<<usrp->get_rx_sensor_names(0)[0]<<std::endl;
    std::cout << boost::format("Sensor value ")<<usrp->get_rx_sensor("lo_locked").to_bool()<<std::endl;

    //set the center frequency
    std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq/1e6) << std::endl << std::endl;
    uhd::tune_request_t tune_request[N];
    int start_freq = freq;

    //creating all the tune requests
    for(int i=0;i<N;i++){
      tune_request[i].target_freq=freq;
      tune_request[i].args = uhd::device_addr_t("mode_n=integer");
      tune_request[i].dsp_freq_policy = uhd::tune_request_t::policy_t(int('A'));
      tune_request[i].rf_freq_policy = uhd::tune_request_t::policy_t(int('A'));
      freq = freq+rate;
    }
    
    uhd::tune_result_t tune_result = usrp->set_rx_freq(tune_request[0]);
    uhd::tune_result_t t_result[N];

    std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq()/1e6) << std::endl << std::endl;
    std::cout << tune_result.to_pp_string()<<std::endl;

    //Reset of the custom FPGA
    usrp->set_user_register(RESET_ADDRESS,1,0);
    usrp->set_user_register(RESET_ADDRESS,0,0);

    //Set threshold of the Energy detector Module
    usrp->set_user_register(TH_ADDRESS,threshold,0);

    //Set the window size of the Energy detector Module
    usrp->set_user_register(WINDOW_SIZE_ADDRESS,window_size-1,0);

    boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup timed

    //create a receive streamer
    uhd::stream_args_t stream_args(type,wirefmt);
    uhd::rx_streamer::sptr rx_stream= usrp->get_rx_stream(stream_args);
    size_t num_rx_samps;//variable used to know the n samples received
    uhd::rx_metadata_t md;
    std::ofstream outfile;
    std::vector< std::complex<int16_t> > buff(spb);
    std::vector<ed_type> buff_ed(spb);
    //setup streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = total_num_samps;
    stream_cmd.stream_now = false;
    double timeout=1;

    //setup plotting
    Gnuplot gp("gnuplot -persist");
    std::ofstream outSQM,outDetect;
    std::ofstream gp_script_file;
    int j=1024;
    std::vector<std::pair<double, double> > xy_pts_A(1024*N);
    std::vector<std::pair<double, double> > xy_pts_B(1024*N);
    int x=0;
    int plot_en =1; // variable to lunch the script for plot
    int plot_freq = start_freq -rate/2; //variable for the frequency axes
    double k,y; // variables for rearranging the data;

    //creating the Gnu Plot script for plot the received data
    gp_script_file.open("gp_script.txt",std::ofstream::out);
    gp_script_file<<"set term wxt 0 \n while(1){ \n set multiplot layout 2, 1 title \"Energy Detection Frequency span\" font \",14\" \n set tmargin 2 \n set title \"|Y|²\" \n unset key \n set xrange ["<<start_freq-rate/2<<":"<<start_freq+(N)*rate-rate/2<< "]\n set format y \"%e\" \n set yrange [-0.1:5]\n plot \"SQM.dat\" binary format=\"%lf\"  with point title 'SQM'\n set title \"Detection\" \n unset key\n set xrange ["<<start_freq-rate/2<<":"<<start_freq+(N)*rate-rate/2<< "]\n set yrange [-0.1:1.1]\n plot \"Detection.dat\" binary format=\"%lf\"  with lines title 'DETECTION'\n unset multiplot\n pause 1\n}\n";
    gp_script_file.close();

    outSQM.open("SQM.dat", std::ofstream::binary);
    outDetect.open("Detection.dat", std::ofstream::binary);

    outSQM.write((const char*)&xy_pts_A.front(),1024*N*sizeof(std::pair<double, double>));
    outDetect.write((const char*)&xy_pts_B.front(),1024*N*sizeof(std::pair<double, double>));
    outSQM.seekp(0);
    outDetect.seekp(0);
    
    std::cout << std::endl;
    std::cout << "Timed commands started ..."<<std::endl;
    //variable set up time commands
    double cmd_time[N];

    int i=0;
    usrp->set_time_now(uhd::time_spec_t(0.0));

    cmd_time[0]= usrp->get_time_now().get_real_secs() + (0.001);

    for(i=1;i<N;i++){
      cmd_time[i]= cmd_time[i-1]+span;
    }

    for( i=0 ; i<N;i++){
      //set the command in time
      usrp->set_command_time(cmd_time[i]);
      t_result[i] = usrp->set_rx_freq(tune_request[i]);
      stream_cmd.time_spec = cmd_time[i]+delta;
      rx_stream->issue_stream_cmd(stream_cmd);
    }

    // for(i=0; i<N;i++){
    //   std::cout << t_result[i].to_pp_string()<<std::endl;
    //   std::cout <<dc_tc[3*i]<<std::endl;
    //   std::cout <<dc_tc[3*i+1]<<std::endl;
    //   std::cout <<dc_tc[3*i+2]<<std::endl;
    // }

    // infinite loop 
    i=0;
    for(; ;){
      
      num_rx_samps = rx_stream->recv(&buff.front(),buff.size(), md,timeout, false);//receive the samples
      
      if(i==0){
	x=0;
	plot_freq = start_freq -rate/2;
	if(usrp->get_time_now().get_real_secs()-cmd_time[N-1]>=span){
	  std::cout<<"i=0"<<std::endl;
	  cmd_time[i]=usrp->get_time_now().get_real_secs()+0.002;
	}
	else{
	   cmd_time[i]=cmd_time[N-1]+span;
	}
      }
      else{
	if(usrp->get_time_now().get_real_secs()-cmd_time[i-1]>=span){
	  std::cout<<i<<std::endl;
	  cmd_time[i]=usrp->get_time_now().get_real_secs()+0.002;	  
	}
	else  
	  cmd_time[i]=cmd_time[i-1]+span;
      }

      usrp->set_command_time(cmd_time[i]);
      usrp->set_rx_freq(tune_request[i]);
      stream_cmd.time_spec = cmd_time[i]+delta;
      rx_stream->issue_stream_cmd(stream_cmd);

      
      if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
	  std::cout << boost::format("Timeout while streaming") << std::endl;
      }
      if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE){
     	std::string error = str(boost::format("Receiver error: %s") % md.strerror());
     	throw std::runtime_error(error);
      }

      for (k=0;k<num_rx_samps;k++){
       buff_ed[k].sq_m = ((uint16_t(buff[k].real())<<17)|(uint16_t(buff[k].imag()<<1)))>>1;
       buff_ed[k].detect = (uint16_t(buff[k].real())>>15);
     }

     for(k=1024/2-1; k>=0; k--) {
       y =log10(buff_ed[k+j].sq_m);
       xy_pts_A[x]=(std::make_pair(plot_freq, y));
       y = buff_ed[k+j].detect;
       xy_pts_B[x]=(std::make_pair(plot_freq, y));
       x++;
       plot_freq = plot_freq + rate/1024;
     }
     for(k=1023; k>1024/2-1; k--) {
       y =log10(buff_ed[k+j].sq_m);
       xy_pts_A[x]=(std::make_pair(plot_freq, y));
       y = buff_ed[k+j].detect;
       xy_pts_B[x]=(std::make_pair(plot_freq, y));
       x++;
       plot_freq = plot_freq + rate/1024;
     }

     outSQM.seekp(x*sizeof(std::pair<double, double>));
     outDetect.seekp(x*sizeof(std::pair<double, double>));
     outSQM.write((const char*)&xy_pts_A[x-1024],1024*sizeof(std::pair<double, double>));
     outDetect.write((const char*)&xy_pts_B[x-1024],1024*sizeof(std::pair<double, double>));
     
     if(plot_en == 1){
       gp<<"load 'gp_script.txt'"<<std::endl;
       plot_en =0;  
     }
     
     i++;
     if(i == N)
       i=0;
    }

    outSQM.close();
    outDetect.close();
    gp <<"quit"<<std::endl;
    //finished

    std::cout << std::endl << "Done!" << std::endl << std::endl;
    
    return EXIT_SUCCESS;

}
