`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    10:44:03 07/22/2017 
// Design Name: 
// Module Name:    FFT_Sq_mag 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module ED_USRP(	
		input 	      clock,
		input 	      reset,
		input 	      strobe_in,
		input 	      set_stb,
		input [7:0]   set_addr,
		input [31:0]  set_data,
		input [31:0]  xn,
		output 	      strobe_out,
		output [31:0] xk
		);

   wire 		      reset_usr;
   

   //FFT unit signals
   wire [15:0] 		      xk_re_fft_core;
   wire [15:0] 		      xk_im_fft_core;
   wire 		      dv_fft_core; 		      

   //sq_m unit signals
   wire [31:0] 		      xk_sq_m;
   wire 		      dv_sq_m;

   //ed unit signals
   wire [31:0] 		      xk_sq_m_dt;
   wire  		      dv_sq_m_dt;
   
   
   //synchronizer signals
   wire 		      dv_synchronizer;
   
   assign strobe_out = dv_synchronizer;

   //User Setting Register definition
   reg [31:0] 			       prtCycles;  // Local register set by User Settings Bus
   wire [31:0] 			       prtCycles_sr;   // setting_reg module output 
   wire 			       prtCycles_ch;     // bit indicating the reg has just changed

   setting_reg #(.my_addr(2)) sr_2        //  0 is the address you choose. (there is an 8 bit address space)
     (.clk(clock),.rst(reset),.strobe(set_stb),.addr(set_addr),.in(set_data),
      .out(prtCycles_sr),.changed(prtCycles_ch));
   
   always @(posedge clock)
     if (prtCycles_ch)
       prtCycles <= prtCycles_sr;
   
   
   assign reset_usr = prtCycles;
   

   FFT FFT_unit(
		.clock(clock),
		.reset(reset|reset_usr),
		.strobe_in(strobe_in),
		.xn(xn),
		.dv_fft_core(dv_fft_core),
		.xk_re_fft_core(xk_re_fft_core),
		.xk_im_fft_core(xk_im_fft_core));
   
   square_mag sq_m_unit(
		  .clock(clock),
		  .reset(reset|reset_usr),
		  .dv_fft(dv_fft_core),
		  .xk_re(xk_re_fft_core),
		  .xk_im(xk_im_fft_core),
		  .dv_sq_m(dv_sq_m),
		  .xk_sq_m(xk_sq_m));

   energy_detection_module ed_unit(
				   .clock(clock),
				   .reset(reset|reset_usr),
				   .set_stb(set_stb),
				   .set_addr(set_addr),
				   .set_data(set_data),
				   .xk_sq_m(xk_sq_m),
				   .dv_sq_m(dv_sq_m),
				   .xk_sq_m_dt(xk_sq_m_dt),
				   .dv_sq_m_dt(dv_sq_m_dt));
   
   data_synchronizer synchronizer_unit (
					.clock(clock), 
					.reset(reset|reset_usr),
					.set_stb(set_stb),
					.set_addr(set_addr),
					.set_data(set_data),		  
					.data_in(xk_sq_m_dt), 
					.dv_in(dv_sq_m_dt), 
					.data_out(xk), 
					.dv_synchronizer(dv_synchronizer));

endmodule
