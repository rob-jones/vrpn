#ifndef	VRPN_BUTTON_H

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "vrpn_Connection.h"
#include "vrpn_BaseClass.h"

#ifndef VRPN_CLIENT_ONLY
#include "vrpn_Serial.h"
#endif

#define	vrpn_BUTTON_MAX_BUTTONS	(100)
const int VRPN_BUTTON_BUF_SIZE = 100;

// Base class for buttons.  Definition
// of remote button class for the user is at the end.

#define vrpn_BUTTON_MOMENTARY	10
#define vrpn_BUTTON_TOGGLE_OFF	20
#define vrpn_BUTTON_TOGGLE_ON	21
#define vrpn_BUTTON_LIGHT_OFF	30
#define vrpn_BUTTON_LIGHT_ON	31
#define vrpn_ALL_ID		-99

class vrpn_Button : public vrpn_BaseClass {
  public:
	vrpn_Button(const char *name, vrpn_Connection *c = NULL);
        virtual ~vrpn_Button (void);

	// Print the status of the button
	void print(void);

	virtual void set_momentary(vrpn_int32 which_button);
        virtual void set_toggle(vrpn_int32 which_button,
				vrpn_int32 current_state);
        virtual void set_all_momentary(void);
        virtual void set_all_toggle(vrpn_int32 default_state);

  protected:
	unsigned char	buttons[vrpn_BUTTON_MAX_BUTTONS];
        unsigned char	lastbuttons[vrpn_BUTTON_MAX_BUTTONS];
	vrpn_int32	minrate[vrpn_BUTTON_MAX_BUTTONS];
	vrpn_int32	num_buttons;
	struct timeval	timestamp;
	vrpn_int32 change_message_id;	// ID of change button message to connection
	vrpn_int32 admin_message_id;	// ID of admin button message to connection

	virtual int register_types (void);
	virtual void report_changes (void);
	virtual vrpn_int32 encode_to(char *buf, vrpn_int32 button,
				     vrpn_int32 state);
};

class vrpn_Button_Filter:public vrpn_Button {
	public:
		vrpn_int32     buttonstate[vrpn_BUTTON_MAX_BUTTONS];
	        virtual void set_momentary(vrpn_int32 which_button);
       		virtual void set_toggle(vrpn_int32 which_button,
					vrpn_int32 current_state);
		virtual void set_all_momentary(void);
		virtual void set_all_toggle(vrpn_int32 default_state);
		void set_alerts(vrpn_int32);

	protected:
		int send_alerts;
		vrpn_Button_Filter(const char *,vrpn_Connection *c=NULL);
		vrpn_int32 alert_message_id;   //used to send back to alert button box for lights
		void report_changes(void);
};

#ifndef VRPN_CLIENT_ONLY

// Button server that lets you set the values for the buttons directly and
// then have it update if needed.  This class should be used by devices that
// can have several sets of buttons in them and don't want to derive from the
// Button class themselves.  An example is the InterSense 900 features found in
// the Fastrak server (which may have several button devices, one for each
// sensor).

class vrpn_Button_Server: public vrpn_Button_Filter {
public:
    vrpn_Button_Server(const char *name, vrpn_Connection *c, int numbuttons = 1);

    /// Tells how many buttons there are (may be clipped to MAX_BUTTONS)
    int number_of_buttons(void);

    /// Called once each time through the server program's mainloop to handle
    /// various functions (like setting toggles, reporting changes, etc).
    virtual void mainloop();

    /// Allows the server program to set current button states (to 0 or 1)
    int	set_button(int button, int new_value);
};


// Example button server code. This button device causes its buttons to
// be pressed and released at the interval specified (default 1/sec). It
// has the specified number of buttons (default 1).
// This class is derived from the vrpn_Button_Filter class, so that it
// can be made to toggle its buttons using messages from the client.

class vrpn_Button_Example_Server: public vrpn_Button_Filter {
public:
	vrpn_Button_Example_Server(const char *name, vrpn_Connection *c,
		int numbuttons = 1, vrpn_float64 rate = 1.0);

	virtual void mainloop();

protected:
	vrpn_float64	_update_rate;	// How often to toggle
};

// Button device that is connected to a parallel port and uses the
// status bits to read from the buttons.  There can be up to 5 buttons
// read this way.
class vrpn_Button_Parallel: public vrpn_Button_Filter {
  public:
	// Open a button connected to the local machine, talk to the
	// outside world through the connection.
	vrpn_Button_Parallel(const char *name, vrpn_Connection *connection,
				int portno);

  protected:
	int	port;
   int	status;

	virtual void read(void) = 0;
#ifdef _WIN32
	int openGiveIO(void);
#endif // _WIN32

};

// Open a Python that is connected to a parallel port on this Linux box.
class vrpn_Button_Python: public vrpn_Button_Parallel {
  public:
	vrpn_Button_Python (const char * name, vrpn_Connection * c, int p);

	virtual void mainloop();
  protected:
  	virtual void read(void);
};


// Button device that is connected to the serial port.
class vrpn_Button_Serial : public vrpn_Button_Filter {
public:
   vrpn_Button_Serial(const char* name, vrpn_Connection *c, 
                      const char *port="/dev/ttyS1/", long baud=38400);
   virtual ~vrpn_Button_Serial();

protected:
   char portname[VRPN_BUTTON_BUF_SIZE];
   long baudrate;
   int serial_fd;
   int	status;

   unsigned char buffer[VRPN_BUTTON_BUF_SIZE]; // char read from the button so far
   vrpn_uint32 bufcount; // number of char in the buffer

   virtual void read()=0;
};

// Open a Fakespace Pinch Glove System that is connected to a serial port. There are
// total of 10 buttons. Buttons 0-4 are fingers for the right hand-thumb first 
// and pinkie last-while buttons 5-9 are for the left hand-thumb first. The report
// you get back is the finger is touching. So you will not have a state where only
// one button is ON.
class vrpn_Button_PinchGlove : public vrpn_Button_Serial {
public:
   vrpn_Button_PinchGlove(const char* name, vrpn_Connection *c, 
                      const char *port="/dev/ttyS1/", long baud=38400);

   virtual void mainloop();

protected:
   virtual void read();
   void report_no_timestamp(); // set the glove to report data without timestamp

private:
   // message constants
   const unsigned char PG_START_BYTE_DATA;
   const unsigned char PG_START_BYTE_DATA_TIME;
   const unsigned char PG_START_BYTE_TEXT;
   const unsigned char PG_END_BYTE;
};
#endif  // VRPN_CLIENT_ONLY


//----------------------------------------------------------
//************** Users deal with the following *************

// User routine to handle a change in button state.  This is called when
// the button callback is called (when a message from its counterpart
// across the connection arrives). The pinch glove has 5 different state of on
// since it knows which fingers are touching.  This pinch glove behavior is
// non-standard and will be removed in a future version.  Button states should
// be considered like booleans.
#define VRPN_BUTTON_OFF	(0)
#define VRPN_BUTTON_ON	(1)

typedef	struct {
	struct timeval	msg_time;	// Time of button press/release
	vrpn_int32	button;		// Which button (numbered from zero)
	vrpn_int32	state;		// button state (0 = off, 1 = on) 
	    // If the button is the type of vrpn_Button_PinchGlove there are up to 5
	    // different kinds of on state since it knows which fingers are touching
} vrpn_BUTTONCB;
typedef void (*vrpn_BUTTONCHANGEHANDLER)(void *userdata,
					 const vrpn_BUTTONCB info);

// Open a button that is on the other end of a connection
// and handle updates from it.  This is the type of button that user code will
// deal with.

class vrpn_Button_Remote: public vrpn_Button {
  public:
	// The name of the button device to connect to. Optional second
	// argument is used when you already have an open connection you
	// want it to listen on.
	vrpn_Button_Remote (const char *name, vrpn_Connection *cn = NULL);
	virtual ~vrpn_Button_Remote (void);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop();

	// (un)Register a callback handler to handle a button state change
	virtual int register_change_handler(void *userdata,
		vrpn_BUTTONCHANGEHANDLER handler);
	virtual int unregister_change_handler(void *userdata,
		vrpn_BUTTONCHANGEHANDLER handler);

  protected:
	typedef	struct vrpn_RBCS {
		void				*userdata;
		vrpn_BUTTONCHANGEHANDLER	handler;
		struct vrpn_RBCS		*next;
	} vrpn_BUTTONCHANGELIST;
	vrpn_BUTTONCHANGELIST	*change_list;

	static int handle_change_message(void *userdata, vrpn_HANDLERPARAM p);
};

#define	VRPN_BUTTON_H
#endif
