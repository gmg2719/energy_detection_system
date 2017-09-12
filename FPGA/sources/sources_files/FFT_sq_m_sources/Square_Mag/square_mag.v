`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    17:17:42 07/18/2017 
// Design Name: 
// Module Name:    square_mag 
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
module square_mag(
		  input 	clock,
		  input 	reset,
		  input 	dv_fft,
		  input [15:0] 	xk_re,
		  input [15:0] 	xk_im,
		  output 	dv_sq_m,
		  output [31:0] xk_sq_m);


   wire [31:0] 			xk_re_sq;
   wire [31:0] 			xk_im_sq;

   wire 			dv_fft_d1,dv_fft_d2,dv_fft_d3;
   wire 			dv_sq,dv_sq_d1,dv_sq_d2,dv_sq_d3;
 			

   dsp48_mul mul_xk_re (
			.clk(clock),
			.sclr(reset),
			.a(xk_re), // Bus [15 : 0] 
			.b(xk_re), // Bus [15 : 0] 
			.p(xk_re_sq)); // Bus [31 : 0] 

   dsp48_mul mul_xk_im (
			.clk(clock),
			.sclr(reset),
			.a(xk_im), // Bus [15 : 0] 
			.b(xk_im), // Bus [15 : 0] 
			.p(xk_im_sq)); // Bus [31 : 0] 

   FFD ffd_fft_d1(
		  .q(dv_fft_d1),
		  .clk(clock),
		  .rst(reset),
		  .d(dv_fft));


   FFD ffd_fft_d2(
		  .q(dv_fft_d2),
		  .clk(clock),
		  .rst(reset),
		  .d(dv_fft_d1));

   FFD ffd_fft_d3(
		  .q(dv_fft_d3),
		  .clk(clock),
		  .rst(reset),
		  .d(dv_fft_d2));   

   
   FFD ffd_fft_d4(
		  .q(dv_sq),
		  .clk(clock),
		  .rst(reset),
		  .d(dv_fft_d3)
		  );

   FFD ffd_sq_d1(
		 .q(dv_sq_d1),
		 .clk(clock),
		 .rst(reset),
		 .d(dv_sq)
		 );
   
   FFD ffd_sq_d2(
		  .q(dv_sq_m),
		  .clk(clock),
		  .rst(reset),
		  .d(dv_sq_d1)
		  );

   /*FFD ffd_sq_d3(
		  .q(dv_sq_d3),
		  .clk(clock),
		  .rst(reset),
		  .d(dv_sq_d2)
		  );

   FFD ffd_sq_d4(
		  .q(dv_sq_m),
		  .clk(clock),
		  .rst(reset),
		  .d(dv_sq_d3)
		  );*/


   add adder_sq_m (
		   .a(xk_re_sq), // Bus [31 : 0] 
		   .b(xk_im_sq), // Bus [31 : 0] 
		   .clk(clock),
		   .sclr(reset),
		   .s(xk_sq_m)); // Bus [31 : 0] 
   
endmodule
