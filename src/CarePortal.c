#include "pebble.h"
#include "stddef.h"
#include "string.h"
#include "pebble_process_info.h"
extern const PebbleProcessInfo __pbl_app_info;
char version[20];

//https://github.com/pebble-examples/feature-simple-menu-layer
#define NUM_MENU_SECTIONS 2
#define NUM_FIRST_MENU_ITEMS 11
#define NUM_SECOND_MENU_ITEMS 1

#define KEY_DATA 5
#define KEY_VALUE 6
#define KEY_EVENT_TYPE 7
#define KEY_EVENTTYPE 8
#define ERROR 9
#define SUCCESS 10
#define DURATION 11
#define PERCENT 12
#define GLUCOSE 13
#define  BG_UNITS 14
#define INSULIN 15
#define SPLITNOW 16
#define SPLITEXT 17
#define PROFILE 18
#define INSULIN_INCREMENT 19
#define CARBS 20

#define MMOL_INTEGER_DEFAULT 5
#define MMOL_FRACTIONAL_DEFAULT 6
#define MGDL_DEFAULT 150
#define CARBS_DEFAULT 20
#define COMBO_BOLUS_COMBO_PERCENTAGE_DEFAULT 100

#define COL_DARK PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack)
#define COL_LIGHT PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite)

#define UP 1
#define DOWN -1
#define INITIAL 0

static Window *s_main_window = NULL;
static Window *carbs_window = NULL;
static Window *insulin_window = NULL;
static Window *populate_window = NULL;
static Window *pumpsitechange_window = NULL;
static Window *tempbasal_window = NULL;
static Window *uploadresult_window = NULL;
static Window *bg_window = NULL;
static Window *combobolus_window = NULL;
static Window *exercise_window = NULL;
static Window *cgmsensor_window = NULL;
static Window *insulinchange_window = NULL;
//static Window *profileswitch_window = NULL;
static Window *carbs_insulin_window = NULL;

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
TextLayer *graph_text_layer_carbs = NULL;
TextLayer *graph_text_layer_insulin = NULL;
TextLayer *graph_text_layer_populate = NULL;
TextLayer *graph_text_layer_pumpsitechange = NULL;
TextLayer *graph_text_layer_TempBasal = NULL;
TextLayer *graph_text_layer_uploadresult = NULL;
TextLayer *graph_text_layer_bg = NULL;
TextLayer *graph_text_layer_combobolus = NULL;
TextLayer* graph_text_layer_exercise = NULL;
TextLayer* graph_text_layer_cgmsensor = NULL;
TextLayer* graph_text_layer_insulinchange = NULL;
//TextLayer* graph_text_layer_profileswitch = NULL;
TextLayer* graph_text_layer_carbs_insulin = NULL;

char messageresultwindow[100];

char outputtext[150];
char fractionaText[10];

char keyname[20];
char resultvalue[60];
char eventtype[40];
char duration[10];
char percent[10];
char unitsused[10];
char bgresult[10];
char insulin[10];
char splitnow[10];
char splitext[10];
char enteredinsulin[10];
char profile[10];

char pumpsitechange[50];
int pumpsiteindex = 0;
static char *pumpsitelocations[9] = { "RHS Stomach", "LHS Stomach", "RHS Bottom", "LHS Bottom","RHS Arm", "LHS Arm", "RHS Leg", "LHS Leg", "Other" };

// General settings
int insulin_increment = 5;
int timestepincrement = 5;
int percentage_increment = 5;

int minutes = 0;
int hrs = 0;
int integerpart_bg = 0;
int fractionalpart_bg = 0;
int integerpart_insulin = 0;
int fractionalpart_insulin = 0;
int icarbs = 0;
int percentage = 0;

// Temp Basal
int iBasalindex = 0; //0 - percentage, 1 - hrs 2- mins
char TempBasal[150];

// for setting insulin numbers - maybe add _insulin to the end.
bool bIntegerPart_set = false;  

// BG variables
bool mmolsunits = true;
static GBitmap *s_menu_icon_image;

// Combo Bolus
int combo_bolus_combo_per = COMBO_BOLUS_COMBO_PERCENTAGE_DEFAULT;
int combo_bolus_currentstep = 0; //0 - integer part, 1 - fractional part, 2 - combo bolus percentage, 3 - hrs, 4 -minutes.

// Exercise
int exercise_index = 0; // 0 - hrs, 1 - minutes

// CGM Sensor
char cgmsensorchange[20];
static char *cgmsensoroptions[2] = { "Sensor Start", "Sensor Change"};
int cgmsensorindex = 0;

// Insulin Change
char insulinchange_change[20];
static char *insulinchangeoptions[2] = { "Reset IAGE"};
int insulinchangeindex = 0;

// Profile Switch
//char profileswitch_change[20];
//static char *profileswitchoptions[3] = { "Holidays", "Weekday", "Weekend"};
//int profileswitchindex = 0;

// carbs and Insulin
int carbs_insulin_index = 0; // 0 - carbs, 1 - insulin integer part, 2 - insulin fractional part



void Set_Carbs(int *current, int increment)
{
	*current += increment;
	if(*current < 0)
		*current = 0;
}

void SetHours(int *current, int change)
{
	*current += change;
	if(*current < 0)
		*current = 0;
}
void SetMinutes(int * current, int change)
{
	if(*current == 0)
	{
		if(change == UP)
		{
			*current += timestepincrement;
		}
		else if(change == DOWN)
		{
			// do  nothing
		}
	}
	else if(*current != 0)
	{
		if(change == UP)
		{
			*current += timestepincrement;
		}
		else if(change == DOWN)
		{
			*current -= timestepincrement;
		}
	}
	
	if(*current == 60)
		*current = 0;
}

// Vibrations
void ShortVibe(){
  vibes_double_pulse();
};

void LongVibe(){
  vibes_long_pulse();
};
/////////////////////////////////////// ERROR HANDLING ///////////////



static void click_config_provider_uploadresult(void *context) {
  // Register the ClickHandlers
 // window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_populate);
}

void uploadresult_load_window(Window * window)
{
    APP_LOG(APP_LOG_LEVEL_INFO, "uploadresult_load_window called!");
    Layer *window_layer_graph = NULL;
    
    window_layer_graph = window_get_root_layer(uploadresult_window);
  #ifdef PBL_ROUND
      graph_text_layer_uploadresult = text_layer_create(GRect(0, 60, 180, 144));
  #else
    graph_text_layer_uploadresult = text_layer_create(GRect(0, 20, 144, 144));
  #endif
    text_layer_set_text_color(graph_text_layer_uploadresult, COL_DARK);
    text_layer_set_background_color(graph_text_layer_uploadresult, COL_LIGHT);
    text_layer_set_text(graph_text_layer_uploadresult, messageresultwindow);
    text_layer_set_font(graph_text_layer_uploadresult, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(graph_text_layer_uploadresult, GTextAlignmentCenter);
    layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_uploadresult));
    
    window_set_click_config_provider(uploadresult_window,(ClickConfigProvider)click_config_provider_uploadresult);
}


void uploadresult_unload_window(Window *window)
{
   if(graph_text_layer_uploadresult)
   {
     text_layer_destroy(graph_text_layer_uploadresult);
   }
  window_destroy(uploadresult_window);
}

void create_uploadresult_window()
{
    uploadresult_window = window_create();
    window_set_window_handlers(uploadresult_window, 
                               (WindowHandlers){
                                 .load   = uploadresult_load_window,
                                 .unload = uploadresult_unload_window,
                               }
                              );  
    window_stack_push(uploadresult_window, true);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback called!");

  // Get the first pair
  Tuple *new_tuple = dict_read_first(iterator);

  // Process all pairs present
  while(new_tuple != NULL)
  {
       switch (new_tuple->key) {
        case ERROR:
        {
            APP_LOG(APP_LOG_LEVEL_INFO, "Error Message: %s", new_tuple->value->cstring);
             
            snprintf(messageresultwindow, sizeof(messageresultwindow), "Error uploading. Please check connection.");
            create_uploadresult_window();
            //add vibe
            LongVibe();
         }
         break;
        case SUCCESS:
         {
           
            APP_LOG(APP_LOG_LEVEL_INFO, "SUCCESSFUL: %s", new_tuple->value->cstring);
            snprintf(messageresultwindow, sizeof(messageresultwindow), "Success uploading to website.");
            create_uploadresult_window();
            //add vibe
            ShortVibe();

         }
         break;
         case BG_UNITS:
         {
           APP_LOG(APP_LOG_LEVEL_INFO, "BG_UNITS: %s", new_tuple->value->cstring);
           snprintf(unitsused, sizeof(unitsused),"%s", new_tuple->value->cstring );
           
           // fixed the mmol to be inline with the mmol for care portal (was mmols)
           char* mmol= "mmol"; 
           if(strstr(unitsused, mmol)!= NULL )
           {
             mmolsunits = true;
             integerpart_bg = MMOL_INTEGER_DEFAULT;
             fractionalpart_bg = MMOL_FRACTIONAL_DEFAULT;
           } 
           else
           {
             mmolsunits = false;
             integerpart_bg = MGDL_DEFAULT;
           }
        }
        break;
		case INSULIN_INCREMENT:
         {
           APP_LOG(APP_LOG_LEVEL_INFO, "INSULIN_INCREMENT: %d", new_tuple->value->int8);
		       insulin_increment = new_tuple->value->int32;
        }
        break;
    }
    // Get next pair, if any
    new_tuple = dict_read_next(iterator);
  }
 }

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "inbox_dropped_callback called!");  
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
   APP_LOG(APP_LOG_LEVEL_INFO, "outbox_failed_callback called!"); 
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback called!");
}

/////////////////////////////////////////////////////////////////////////////////////

// function used in incrementing the insulin count integer value.
// if the value is less than 0 set it to 0.
void Set_IntegerPart(int* current, int increment)
{
    *current+= increment;  
    if(*current < 0)
    {
      *current = 0;
    }
 }

// function used to set the decimal value for the insulin count
// possible todo:  change to similar function as BG for mmol that increments from decimal entirely and not 2 stage
void Set_FractionPart(int* current, int increment)
{
  *current += increment;
  if(*current < 0)
  {
    *current = 0;
  }
  else if(*current >=100)
  {
      *current = 0;
  }
}

// function to return the 2 digit decimal as 2 characters if the decimal value is less than 2 digits  (.00, or .05)
char* GetFractionaPartAsChar(int currentvalue) 
{
	if(currentvalue<=9)
	{
		snprintf(fractionaText, sizeof(fractionaText), "0%d", currentvalue);
	}
	else
	{
		snprintf(fractionaText, sizeof(fractionaText), "%d", currentvalue);
	}
  return fractionaText;
}

// determines the BG settings for the mg/dl settings.  mmol now uses just the decimal incrementer.
void Set_IntegerPart_BG(int * current, int increment)
{
    *current+= increment;  
    if(*current < 0)
    {
      *current = 0;
    }
}

// incremental change to the decimal section of BG.  Rolls over integer section on boundaries (>9, increment integer, <0 decrement integer if it is not 0)
void Set_FractionPart_BG(int *currentintegerpart, int *current, int increment)
{
  *current += increment;
  if(*current < 0)
  {
      if (*currentintegerpart > 0)
      { 
        *currentintegerpart= *currentintegerpart - 1;
        *current = 9; 
      }
      else 
      {
        *current = 0; 
      }
  }
  else if(*current >=10)
  {
    *currentintegerpart = *currentintegerpart  + 1;  
    *current = 0;
  }
}

// determines if it should be doing the decimal increments for mmol or the integer increment for mg/dl
void Set_Part_BG(bool currentPartSet, int increment) {

  if(mmolsunits)
  {
    Set_FractionPart_BG(&integerpart_bg, &fractionalpart_bg, increment);
  }
  else  
  {
    Set_IntegerPart_BG(&integerpart_bg,increment);
  }
}

// returns the string of the new site location selection
char* GetPumpSiteChangeLocation(int change)
{
   pumpsiteindex += change;
  
   int count = sizeof(pumpsitelocations)/sizeof(*pumpsitelocations);
   app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###GetPumpSiteChangeLocation: count: %d ###", count);
   if(pumpsiteindex >= count)
   {
     pumpsiteindex = 0;
   }
   else if(pumpsiteindex < 0)
   {
     pumpsiteindex = count - 1;      
   } 
   snprintf(pumpsitechange, sizeof(pumpsitechange), "%s", pumpsitelocations[pumpsiteindex]);
   return pumpsitechange;
}

// return all variables to the initial values 
void ResetToDefaults()
{
  memset(keyname, 0, sizeof(keyname));
  memset(resultvalue, 0, sizeof(resultvalue));
  bIntegerPart_set = false;
  icarbs = 0;
  integerpart_insulin = 0;
  fractionalpart_insulin = 0;
  hrs = 0;
  minutes = 0;
  percentage = 0;
  
  // Temp Basal
  iBasalindex = 0;
  
  // reset memory allocation and initialise back to zero length
  memset(duration, 0, sizeof(duration));
  memset(percent, 0, sizeof(percent));
  
  // BG resets
  memset(bgresult, 0, sizeof(bgresult));
   
  // output text reset
  memset(outputtext, 0, sizeof(outputtext));
  
  // set both mmol and mg/dl defaults
  if(mmolsunits)
  {  
    integerpart_bg = MMOL_INTEGER_DEFAULT;
    fractionalpart_bg = MMOL_FRACTIONAL_DEFAULT;  
  }
  else
  {
    integerpart_bg = MGDL_DEFAULT;
  }
  
  //combo bolus
   combo_bolus_combo_per = COMBO_BOLUS_COMBO_PERCENTAGE_DEFAULT;
   combo_bolus_currentstep = 0; 
  
  // Exercise
	exercise_index = 0;
	
	// CGM Sensor
	cgmsensorindex = 0;
	
	// Insulin CHANGE
	insulinchangeindex = 0;
	
	// Profile Switch
	//profileswitchindex = 0;
	
	// Carbs and insulin
	carbs_insulin_index = 0;	
	
	memset(eventtype, 0, sizeof(eventtype));
	memset(insulin, 0, sizeof(insulin));
	memset(splitnow, 0, sizeof(splitnow));
	memset(splitext, 0, sizeof(splitext));
	memset(enteredinsulin, 0, sizeof(enteredinsulin));
	//memset(profile, 0, sizeof(profile));
  }

//////////////////////// POPULATE WINDOW ///////////////////////////////////////
void select_click_handler_populate(ClickRecognizerRef recognizer, void *context) {
   DictionaryIterator *iter;
   AppMessageResult result = app_message_outbox_begin(&iter);
  
   if(result == APP_MSG_OK)
    { 
        if(strlen(keyname) != 0)
        {
            dict_write_cstring(iter, KEY_DATA, &keyname[0]);
        }

        if(strlen(resultvalue) != 0)
        {
            dict_write_cstring(iter, KEY_VALUE, &resultvalue[0]);
        }

        if(strlen(eventtype) != 0)
        {
            dict_write_cstring(iter, KEY_EVENTTYPE, &eventtype[0]);
        }

        if(strlen(duration) != 0)
        {
            dict_write_cstring(iter, DURATION, &duration[0]);
        }

        if(strlen(percent) != 0)
        {
            dict_write_cstring(iter, PERCENT, &percent[0]);
        }

        if(strlen(bgresult) != 0)
        {
            dict_write_cstring(iter, GLUCOSE, &bgresult[0]);
        }
        
        if(strlen(insulin) != 0)
        {
            app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: insulin: %s ###", insulin);
            dict_write_cstring(iter, INSULIN, &insulin[0]);
        }        
        
        if(strlen(splitnow) != 0)
        {
            app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: splitnow: %s ###", splitnow);
            dict_write_cstring(iter, SPLITNOW, &splitnow[0]);
        }
        
        if(strlen(splitext) != 0)
        {
           app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: splitext: %s ###", splitext);
           dict_write_cstring(iter, SPLITEXT, &splitext[0]);
        }
     
        if(strlen(profile) != 0)
		    {
           app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: profile: %s ###", profile);
           dict_write_cstring(iter, PROFILE, &profile[0]);
        }
        app_message_outbox_send();

        window_stack_pop_all(true);
        ResetToDefaults();
        window_stack_push(s_main_window, true);
    }

    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_populate: Exiting###");
}


static void click_config_provider_populate(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_populate);
}

void populate_load_window(Window * window)
{
    Layer *window_layer_graph = NULL;
    
    window_layer_graph = window_get_root_layer(populate_window);
  #ifdef PBL_ROUND
    graph_text_layer_populate = text_layer_create(GRect(0, 60, 180, 144));
  #else
    graph_text_layer_populate = text_layer_create(GRect(0, 20, 144, 144));
  #endif
    text_layer_set_text(graph_text_layer_populate, outputtext);
    text_layer_set_text_color(graph_text_layer_populate, COL_DARK);
    text_layer_set_background_color(graph_text_layer_populate, COL_LIGHT);
    text_layer_set_font(graph_text_layer_populate, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(graph_text_layer_populate, GTextAlignmentCenter);
    layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_populate));
    
    window_set_click_config_provider(populate_window,(ClickConfigProvider)click_config_provider_populate);
}


void populate_unload_window(Window *window)
{
   if(graph_text_layer_populate)
   {
     text_layer_destroy(graph_text_layer_populate);
   }
  window_destroy(populate_window);
  
}

void create_populate_window()
{
   populate_window = window_create();
    	  window_set_window_handlers(populate_window, 
    							   (WindowHandlers){
												.load   = populate_load_window,
											  .unload = populate_unload_window,
										}
    							   );  
    	
  
    	  window_stack_push(populate_window, true);
}

/////////////////////START OF CARBS AND INSULIN //////////////////////
void Set_Carbs_InsulinPart(int index, int buttonpress )
{
	if(index == 0) // Carbs
	{
    Set_Carbs(&icarbs, buttonpress);
	//	icarbs += buttonpress;
	}
	else if(index == 1) // Insulin integer part
	{
		Set_IntegerPart(&integerpart_insulin, buttonpress);
	}
	else if(index == 2) // insulin fractional part
	{
		Set_FractionPart(&fractionalpart_insulin, insulin_increment * buttonpress);
	}
}

void Set_GraphText_layer_carbs_Insulin(TextLayer* currentlayer, int currentindex, int buttonpress)
{
  Set_Carbs_InsulinPart(currentindex, buttonpress);
  static char s_packet_id_text[60];
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Carbs: %dg\n Insulin: %d.%s units", icarbs, integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin));
  text_layer_set_text(currentlayer, s_packet_id_text);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###Set_GraphText_layer_carbs_Insulin: Exiting###");
}

static void select_click_handler_carbs_insulin(ClickRecognizerRef recognizer, void *context) {
    if(carbs_insulin_index != 2)
    {
      carbs_insulin_index +=  1; 
    }
    else
    {
        snprintf(outputtext, sizeof(outputtext), "You are adding 'Carbs: %dg\nInsulin: %d.%s units'  to Care Portal.", icarbs, integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin));

        snprintf(keyname, sizeof(keyname), "carbs");
	      snprintf(resultvalue, sizeof(resultvalue), "%d", icarbs);
		    snprintf(insulin, sizeof(insulin), "%d.%s", integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin));
        //snprintf(resultvalue, sizeof(resultvalue), "Carbs: %d g\nInsulin: %d.%s units", icarbs, integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin));
        snprintf(eventtype,sizeof(eventtype), "Note");
        create_populate_window();
        carbs_insulin_index = 0;
      
      /*
      snprintf(outputtext, sizeof(outputtext), "You are adding 'Carbs: %d g'  to Care Portal.", icarbs);
  snprintf(keyname, sizeof(keyname), "carbs");
  snprintf(resultvalue, sizeof(resultvalue), "%d", icarbs);
  snprintf(eventtype,sizeof(eventtype), "Note");
  */
      
    }
}

static void up_click_handler_carbs_insulin(ClickRecognizerRef recognizer, void *context) { 
	 
    Set_GraphText_layer_carbs_Insulin(graph_text_layer_carbs_insulin,carbs_insulin_index, UP);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_carbs_insulin: Exiting###");
}


static void down_click_handler_carbs_insulin(ClickRecognizerRef recognizer, void *context) {
   
    Set_GraphText_layer_carbs_Insulin(graph_text_layer_carbs_insulin,carbs_insulin_index, DOWN);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_carbs_insulin: Exiting###");
}

static void click_config_provider_carbs_insulin(void *context) {
  // Register the ClickHandlers
   window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_carbs_insulin);
  // using repeated clicks to scroll quickly through numbers instead of long click that has to be repressed to increment by 10.  Scrolls through 10 values / second
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 60, up_click_handler_carbs_insulin);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 60, down_click_handler_carbs_insulin);
}

void carbs_insulin_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(carbs_insulin_window);
#ifdef PBL_ROUND
  graph_text_layer_carbs_insulin = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_carbs_insulin = text_layer_create(GRect(0, 20, 144, 170));
#endif
  //"Carbs: %d g \n Insulin: %d.%s units", icarbs, integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin)
  Set_GraphText_layer_carbs_Insulin(graph_text_layer_carbs_insulin, carbs_insulin_index, INITIAL);
 // text_layer_set_text(graph_text_layer_carbs_insulin, "Carbs: 0g\nInsulin: 0.00 units");
  text_layer_set_text_color(graph_text_layer_carbs_insulin, COL_DARK);
  text_layer_set_background_color(graph_text_layer_carbs_insulin, COL_LIGHT);
  text_layer_set_font(graph_text_layer_carbs_insulin, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_carbs_insulin, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_carbs_insulin));
  
  window_set_click_config_provider(carbs_insulin_window,(ClickConfigProvider)click_config_provider_carbs_insulin);
   app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###carbs_insulin_load_graph: Exiting###");
}

void carbs_insulin_unload_graph(Window *window) {
   if(graph_text_layer_carbs_insulin)
   {
     text_layer_destroy(graph_text_layer_carbs_insulin);
   }
   window_destroy(carbs_insulin_window);
}
/////////////////////END OF CARBS AND INSULIN //////////////////////




///////////////////// INSULIN //////////////////////
// determines if the insulin setting is doing the integer section or the decimal section and sets the appropriate value
// possible todo: bring in line with the BG increment from decimal feature
void Set_Part(bool currentPartSet, int increment)
{
    if(!currentPartSet)
    {      
       Set_IntegerPart(&integerpart_insulin, increment);
    }
    else
    {
	   Set_FractionPart(&fractionalpart_insulin, insulin_increment * increment);
    }
}

void Set_GraphText_layer_Insulin(TextLayer* currentlayer, bool currentPartSet, int increment)
{
  Set_Part(bIntegerPart_set, increment);
  static char s_packet_id_text[30];
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Insulin: %d.%s units", integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin));
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void select_click_handler_insulin(ClickRecognizerRef recognizer, void *context) {
    if(!bIntegerPart_set)
    {
      bIntegerPart_set = true;  
    }
    else
    {
        snprintf(outputtext, sizeof(outputtext), "You are adding 'Insulin: %d.%s units'  to Care Portal.", integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin));

        snprintf(keyname, sizeof(keyname), "insulin");
        snprintf(resultvalue, sizeof(resultvalue), "%d.%s", integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin));
        snprintf(eventtype,sizeof(eventtype), "Note");
        create_populate_window();
        bIntegerPart_set = false;
    }
}

static void up_click_handler_insulin(ClickRecognizerRef recognizer, void *context) { 
	 
    Set_GraphText_layer_Insulin(graph_text_layer_insulin,bIntegerPart_set, UP);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_insulin: : Exiting###");
}


static void down_click_handler_insulin(ClickRecognizerRef recognizer, void *context) {
   
    Set_GraphText_layer_Insulin(graph_text_layer_insulin,bIntegerPart_set, DOWN);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_insulin: Exiting###");
}

static void click_config_provider_insulin(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_insulin);
  // using repeated clicks to scroll quickly through numbers instead of long click that has to be repressed to increment by 10.  Scrolls through 10 values / second
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 60, up_click_handler_insulin);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 60, down_click_handler_insulin);
}

void insulin_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(insulin_window);
 #ifdef PBL_ROUND
  graph_text_layer_insulin = text_layer_create(GRect(0, 75, 180, 27));
#else
  graph_text_layer_insulin = text_layer_create(GRect(0, 20, 144, 27));
#endif  
  text_layer_set_text(graph_text_layer_insulin, "Insulin: 0.00 units");
  text_layer_set_text_color(graph_text_layer_insulin, COL_DARK);
  text_layer_set_background_color(graph_text_layer_insulin, COL_LIGHT);
  text_layer_set_font(graph_text_layer_insulin, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_insulin, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_insulin));
  
  window_set_click_config_provider(insulin_window,(ClickConfigProvider)click_config_provider_insulin);
}

void insulin_unload_graph(Window *window) {
   if(graph_text_layer_insulin)
   {
     text_layer_destroy(graph_text_layer_insulin);
   }
   window_destroy(insulin_window);
}

////////////////////// CARBS WINDOW///////////////////////////////////////////////////////////

void Set_GraphText_layer_carbs(TextLayer* currentlayer, int increment)
{
  Set_Carbs(&icarbs, increment);
 
  static char s_packet_id_text[20];
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Carbs: %d g", icarbs);
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_carbs(ClickRecognizerRef recognizer, void *context) { 
  Set_GraphText_layer_carbs(graph_text_layer_carbs, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_carbs: up_click_handler_carbs###");
}

static void select_click_handler_carbs(ClickRecognizerRef recognizer, void *context) {
  snprintf(outputtext, sizeof(outputtext), "You are adding 'Carbs: %d g'  to Care Portal.", icarbs);
  snprintf(keyname, sizeof(keyname), "carbs");
  snprintf(resultvalue, sizeof(resultvalue), "%d", icarbs);
  snprintf(eventtype,sizeof(eventtype), "Note");

  create_populate_window();
}

static void down_click_handler_carbs(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_carbs(graph_text_layer_carbs, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_carbs: down_click_handler_carbs###");
}

static void click_config_provider_carbs(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_carbs);
  // using repeated clicks to scroll quickly through numbers instead of long click that has to be repressed to increment by 10.  Scrolls through 10 values / second
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 30, up_click_handler_carbs);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 30, down_click_handler_carbs);
}

void carbs_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(carbs_window);
 #ifdef PBL_ROUND
  graph_text_layer_carbs = text_layer_create(GRect(0, 75, 180, 37));
 #else
 graph_text_layer_carbs = text_layer_create(GRect(0, 20, 144, 37));
#endif
  text_layer_set_text_color(graph_text_layer_carbs, COL_DARK);
  text_layer_set_text(graph_text_layer_carbs, "Carbs: 0 g");
  text_layer_set_background_color(graph_text_layer_carbs, COL_LIGHT);
  text_layer_set_font(graph_text_layer_carbs, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_carbs, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_carbs));
  
  window_set_click_config_provider(carbs_window,(ClickConfigProvider)click_config_provider_carbs);
}

void carbs_unload_graph(Window *window) {
  
   if(graph_text_layer_carbs)
   {
     text_layer_destroy(graph_text_layer_carbs);
   }
   window_destroy(carbs_window);
}


//////// PUMP SITE CHANGE ////////////////////////////////////////////////////////////////
void Set_GraphText_layer_pumpsitechange(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];

  char * sitechange = GetPumpSiteChangeLocation(change);
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Pump Site Location: %s", sitechange);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "Pump Site Location: %s", sitechange);
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) { 
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_pumpsitechange: Exiting###");
}

static void select_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) {
    char * sitechange = GetPumpSiteChangeLocation(INITIAL);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_pumpsitechange - Pump Site Location: %s", sitechange);
    snprintf(outputtext, sizeof(outputtext), "You are adding 'Pump Site Location: %s'  to Care Portal.", sitechange);
    snprintf(keyname, sizeof(keyname), "notes");
    snprintf(resultvalue, sizeof(resultvalue), "%s", sitechange);
    snprintf(eventtype,sizeof(eventtype), "Site Change");
  
    create_populate_window();
}

static void down_click_handler_pumpsitechange(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_pumpsitechange: Exiting###");
}

static void click_config_provider_pumpsitechange(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_pumpsitechange);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_pumpsitechange);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_pumpsitechange);
}

void pumpsitechange_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(pumpsitechange_window);
#ifdef PBL_ROUND
  graph_text_layer_pumpsitechange = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_pumpsitechange = text_layer_create(GRect(0, 20, 144, 170));
#endif
 
  Set_GraphText_layer_pumpsitechange(graph_text_layer_pumpsitechange, INITIAL);
  text_layer_set_text_color(graph_text_layer_pumpsitechange, COL_DARK);
  text_layer_set_background_color(graph_text_layer_pumpsitechange, COL_LIGHT);
  text_layer_set_font(graph_text_layer_pumpsitechange, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_pumpsitechange, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_pumpsitechange));
  
  window_set_click_config_provider(pumpsitechange_window,(ClickConfigProvider)click_config_provider_pumpsitechange);
}

void pumpsitechange_unload_graph(Window *window) {
   if(graph_text_layer_pumpsitechange)
   {
     text_layer_destroy(graph_text_layer_pumpsitechange);
   }
   pumpsiteindex =0;
   window_destroy(pumpsitechange_window);
}
/////////////////////// END OF PUMP SITE CHANGE ///////////////////////////////

///////////////////// START OF TEMP BASAL ////////////////////////////////////
void SetTempBasalData(int change)
{
    if(iBasalindex == 0)
    {
      percentage += percentage_increment * change;
      if(percentage < -100)
        percentage = -100;
      else if (percentage > 200)
        percentage = 200;
    }
	else if(iBasalindex == 1)
	{
		SetHours(&hrs, change);
	}
    else if(iBasalindex == 2)
    {
        SetMinutes(&minutes, change);
    }
}

void Set_GraphText_layer_TempBasal(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];
 
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "TempBasal: %+d%% \n over %d hrs %d mins", percentage, hrs, minutes);

  text_layer_set_text(currentlayer, s_packet_id_text);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###Set_GraphText_layer_TempBasal: Exiting###");
}

static void up_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) { 
 
  SetTempBasalData(UP);  
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, UP);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_TempBasal: Exiting###");
}

static void select_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) {
    
  if(iBasalindex < 2)
  {
      iBasalindex += 1;
  }
  else
  {
      snprintf(outputtext, sizeof(outputtext), "You are adding 'TempBasal' %+d%% over %d hrs %d mins to Care Portal.", percentage, hrs, minutes);
      snprintf(keyname, sizeof(keyname), "notes");
      snprintf(resultvalue, sizeof(resultvalue), "Temp Basal %+d%% over %d hrs %d mins.", percentage, hrs, minutes);
      snprintf(eventtype,sizeof(eventtype), "Temp Basal");
      snprintf(duration,sizeof(duration), "%d",(hrs * 60) + minutes );
      snprintf(percent,sizeof(percent), "%d", percentage);
    
      create_populate_window();
  }
}

static void down_click_handler_TempBasal(ClickRecognizerRef recognizer, void *context) {
  SetTempBasalData(DOWN);  
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_TempBasal: Exiting###");
}


static void click_config_provider_TempBasal(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_TempBasal);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_TempBasal);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_TempBasal);
}

void tempbasal_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(tempbasal_window);
#ifdef PBL_ROUND
  graph_text_layer_TempBasal = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_TempBasal = text_layer_create(GRect(0, 20, 144, 170));
#endif
  Set_GraphText_layer_TempBasal(graph_text_layer_TempBasal, INITIAL);
  
  text_layer_set_text_color(graph_text_layer_TempBasal, COL_DARK);
  text_layer_set_background_color(graph_text_layer_TempBasal, COL_LIGHT);
  text_layer_set_font(graph_text_layer_TempBasal, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_TempBasal, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_TempBasal));
  
  window_set_click_config_provider(tempbasal_window,(ClickConfigProvider)click_config_provider_TempBasal);
}

void tempbasal_unload_graph(Window *window) {
   if(graph_text_layer_TempBasal)
   {
     text_layer_destroy(graph_text_layer_TempBasal);
   } 
  
   ResetToDefaults();
   window_destroy(tempbasal_window);
}

///////////////////// END OF Temp Basal CHANGE ///////////////////////////////

///////////////////// Start of glucose //////////////////////

// removed the currentPartSet as we're now setting decimals to start with for mmol
void Set_GraphText_layer_bg(TextLayer* currentlayer, int increment)
{
  Set_Part_BG(bIntegerPart_set, increment);
 // int temp_integerpart = integerpart_bg;
  static char s_packet_id_text[30];
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###Set_GraphText_layer_bg unitsused: %s", unitsused);
  //unitsused
  if(mmolsunits)
  {
      snprintf(s_packet_id_text, sizeof(s_packet_id_text), "BG: %d.%d %s", integerpart_bg, fractionalpart_bg, unitsused);
  }
  else
  {
      snprintf(s_packet_id_text, sizeof(s_packet_id_text), "BG: %d %s", integerpart_bg, unitsused);
  }
  text_layer_set_text(currentlayer, s_packet_id_text);
   app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###Set_GraphText_layer_bg: : Exiting###");
}

static void select_click_handler_bg(ClickRecognizerRef recognizer, void *context) {
  if(mmolsunits)
  {
    snprintf(outputtext, sizeof(outputtext), "You are adding 'BG result: %d.%d %s'  to Care Portal.", integerpart_bg, fractionalpart_bg, unitsused);
    snprintf(bgresult, sizeof(bgresult), "%d.%d", integerpart_bg,fractionalpart_bg );
  }
  else
  {
    snprintf(outputtext, sizeof(outputtext), "You are adding 'BG result: %d %s'  to Care Portal.", integerpart_bg,unitsused);
    snprintf(bgresult, sizeof(bgresult), "%d", integerpart_bg);
  }
  snprintf(eventtype,sizeof(eventtype), "BG Check");
  create_populate_window();
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###select_click_handler_bg: : Exiting###");
}

static void up_click_handler_bg(ClickRecognizerRef recognizer, void *context) { 
    Set_GraphText_layer_bg(graph_text_layer_bg, UP);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_bg: Exiting###");
}

static void down_click_handler_bg(ClickRecognizerRef recognizer, void *context) {
    Set_GraphText_layer_bg(graph_text_layer_bg, DOWN);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_bg: Exiting###");
}

static void click_config_provider_bg(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_bg);
  // using repeated clicks to scroll quickly through numbers instead of long click that has to be repressed to increment by 10.  Scrolls through 10 values / second
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_bg);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_bg);
}
void bg_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(bg_window);
#ifdef PBL_ROUND
  graph_text_layer_bg = text_layer_create(GRect(0, 75, 180, 27));
#else
  graph_text_layer_bg = text_layer_create(GRect(0, 20, 144, 27));
#endif
  static char s_packet_id_text[60];
  if(mmolsunits)
  {
     snprintf(s_packet_id_text, sizeof(s_packet_id_text), "BG: %d.%d %s", integerpart_bg, fractionalpart_bg, unitsused);
  }
  else
  {
      snprintf(s_packet_id_text, sizeof(s_packet_id_text), "BG: %d %s", integerpart_bg, unitsused);
  }

  text_layer_set_text(graph_text_layer_bg, s_packet_id_text);
  text_layer_set_text_color(graph_text_layer_bg, COL_DARK);
  text_layer_set_background_color(graph_text_layer_bg, COL_LIGHT);
  text_layer_set_font(graph_text_layer_bg, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_bg, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_bg));
  
  window_set_click_config_provider(bg_window,(ClickConfigProvider)click_config_provider_bg);
}

void bg_unload_graph(Window *window) {
   if(graph_text_layer_bg)
   {
     text_layer_destroy(graph_text_layer_bg);
   }
   window_destroy(bg_window);
}
////////////end of bg mmol window///////////////

/////////// COMBO BOLUS WINDOW///////////////
void UpdateComboBolusDetails(int change)
{
   // app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails: %d", combo_bolus_currentstep);
    if(combo_bolus_currentstep == 0) // insulin integer part
    {
        //app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails- integerpart_insulin: %d", integerpart_insulin);
        Set_IntegerPart(&integerpart_insulin, change);

    }
    else if(combo_bolus_currentstep == 1) // insulin Fractional part
    {
     //  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateComboBolusDetails- combo_bolus_insulin_combo_bolus_insulin_fractionalintegerpart: %d", fractionalpart_insulin);
	    Set_FractionPart(&fractionalpart_insulin, insulin_increment* change);
    }
    else if(combo_bolus_currentstep == 2) // Percentage
    {
		combo_bolus_combo_per += percentage_increment * change;
		
        if(combo_bolus_combo_per > 100)
			combo_bolus_combo_per = 100;
		
		if (combo_bolus_combo_per < 0)
			combo_bolus_combo_per = 0;
    }
	else if(combo_bolus_currentstep == 3) // hrs
    {
		SetHours(&hrs, change);
	}
    else if(combo_bolus_currentstep == 4) // minutes
    {
       SetMinutes(&minutes, change);
    }
}

void Set_GraphText_layer_combobolus(TextLayer* currentlayer, int change)
{
    static char s_packet_id_text[70];
    UpdateComboBolusDetails(change);
    snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Insulin: %d.%s units.\nCombo bolus\n %d%%/%d%%\n over\n %d hrs %d mins",
        integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin), combo_bolus_combo_per, (100 - combo_bolus_combo_per),hrs, minutes );
    
    text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_combobolus(ClickRecognizerRef recognizer, void *context) {
    Set_GraphText_layer_combobolus(graph_text_layer_combobolus, UP);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_combobolus: Exiting###");
}

static void select_click_handler_combobolus(ClickRecognizerRef recognizer, void *context) {
 
    if(combo_bolus_currentstep < 4)
    {
        combo_bolus_currentstep += 1;
    }
    else
    {
        app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_combobolus");
		int precentagedifference = 100 - combo_bolus_combo_per;
		
        snprintf(outputtext, sizeof(outputtext), "You are adding Insulin: %d.%s units.\nCombo bolus\n %d%%/%d%%\n over\n %d hrs %d minutes",
            integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin), combo_bolus_combo_per, precentagedifference, hrs, minutes );

        snprintf(keyname, sizeof(keyname), "notes");
        snprintf(resultvalue, sizeof(resultvalue),  "COMBO BOLUS");
        snprintf(eventtype,sizeof(eventtype), "Combo Bolus");
        snprintf(duration,sizeof(duration), "%d",(hrs * 60) + minutes );
        snprintf(splitnow,sizeof(splitnow), "%d",combo_bolus_combo_per );
        snprintf(splitext,sizeof(splitext), "%d",precentagedifference);
        snprintf(insulin,sizeof(insulin), "%d.%s",integerpart_insulin, GetFractionaPartAsChar(fractionalpart_insulin) );
      
        create_populate_window();
        combo_bolus_currentstep = 0;
    }
}

static void down_click_handler_combobolus(ClickRecognizerRef recognizer, void *context) {
    Set_GraphText_layer_combobolus(graph_text_layer_combobolus,DOWN);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_combobolus: Exiting###");
}

static void click_config_provider_combobolus(void *context) {
    // Register the ClickHandlers
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_combobolus);
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_combobolus);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_combobolus);
}

void combobolus_load_graph(Window *window) {

    ResetToDefaults();
    Layer *window_layer_graph = NULL;

    window_layer_graph = window_get_root_layer(combobolus_window);
#ifdef PBL_ROUND
    graph_text_layer_combobolus = text_layer_create(GRect(0, 60, 180, 170));
#else
    graph_text_layer_combobolus = text_layer_create(GRect(0, 20, 144, 170));
#endif

    Set_GraphText_layer_combobolus(graph_text_layer_combobolus, INITIAL);
    text_layer_set_text_color(graph_text_layer_combobolus, COL_DARK);
    text_layer_set_background_color(graph_text_layer_combobolus, COL_LIGHT);
    text_layer_set_font(graph_text_layer_combobolus, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(graph_text_layer_combobolus, GTextAlignmentCenter);
    layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_combobolus));

    window_set_click_config_provider(combobolus_window,(ClickConfigProvider)click_config_provider_combobolus);

}

void combobolus_unload_graph(Window *window) {
    if(graph_text_layer_combobolus)
    {
        text_layer_destroy(graph_text_layer_combobolus);
    }

    window_destroy(combobolus_window);
}

////////////End of Combo Bolus Window


////////////////////// EXERCISE WINDOW///////////////////////////////////////////////////////////
void UpdateExerciseDetails(int index, int change)
{
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "UpdateExerciseDetails: %d", change);
    
	if(index == 0) // Hrs
	{
		SetHours(&hrs, change);
	}
    else // mins
	{
		SetMinutes(&minutes, change);
	}
}

void Set_GraphText_layer_exercise(TextLayer* currentlayer, int change)
{
    static char s_packet_id_text[60];
    UpdateExerciseDetails(exercise_index, change);

    snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Exercise:%d hrs\n%d minutes", hrs, minutes);
    text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_exercise(ClickRecognizerRef recognizer, void *context) {
    Set_GraphText_layer_exercise(graph_text_layer_exercise, UP);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_exercise: Exiting###");
}

static void select_click_handler_exercise(ClickRecognizerRef recognizer, void *context) {
 
    if(exercise_index == 0)
	{
		exercise_index+= 1;
	}
    else
	{
		app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_exercise");
		snprintf(outputtext, sizeof(outputtext), "You are adding Exercise:%d hrs\n%d minutes", hrs, minutes );

		snprintf(keyname, sizeof(keyname), "notes");
		snprintf(resultvalue, sizeof(resultvalue),  "Exercise Added");
		snprintf(eventtype,sizeof(eventtype), "Exercise");
		snprintf(duration,sizeof(duration), "%d",(hrs * 60) + minutes );
		
		create_populate_window();
		exercise_index = 0;
	}
}

static void down_click_handler_exercise(ClickRecognizerRef recognizer, void *context) {
    Set_GraphText_layer_exercise(graph_text_layer_exercise,DOWN);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_exercise: Exiting###");
}


static void click_config_provider_exercise(void *context) {
    // Register the ClickHandlers
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_exercise);
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler_exercise);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler_exercise);
}

void exercise_load_graph(Window *window) {

    ResetToDefaults();
    Layer *window_layer_graph = NULL;

    window_layer_graph = window_get_root_layer(exercise_window);
#ifdef PBL_ROUND
    graph_text_layer_exercise = text_layer_create(GRect(0, 60, 180, 170));
#else
    graph_text_layer_exercise = text_layer_create(GRect(0, 20, 144, 170));
#endif

    Set_GraphText_layer_exercise(graph_text_layer_exercise, INITIAL);
    text_layer_set_text_color(graph_text_layer_exercise, COL_DARK);
    text_layer_set_background_color(graph_text_layer_exercise, COL_LIGHT);
    text_layer_set_font(graph_text_layer_exercise, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(graph_text_layer_exercise, GTextAlignmentCenter);
    layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_exercise));

    window_set_click_config_provider(exercise_window,(ClickConfigProvider)click_config_provider_exercise);

}

void exercise_unload_graph(Window *window) {
    if(graph_text_layer_exercise)
    {
        text_layer_destroy(graph_text_layer_exercise);
    }

    window_destroy(exercise_window);
}
//////// CGM SENSOR ////////////////////////////////////////////////////////////////
// returns the string of the new site location selection
char* GetCGMSensorOption(int change)
{
   cgmsensorindex += change;
  
   int count = sizeof(cgmsensoroptions)/sizeof(*cgmsensoroptions);
   app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###GetCGMSensorOption: count: %d ###", count);
   if(cgmsensorindex >= count)
   {
     cgmsensorindex = 0;
   }
   else if(cgmsensorindex < 0)
   {
     cgmsensorindex = count - 1;      
   } 
   snprintf(cgmsensorchange, sizeof(cgmsensorchange), "%s", cgmsensoroptions[cgmsensorindex]);
   return cgmsensorchange;
}



void Set_GraphText_layer_cgmsensor(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];

  char * cgmchange = GetCGMSensorOption(change);
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "CGM Sensor option: %s", cgmchange);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "CGM Sensor option: %s", cgmchange);
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_cgmsensor(ClickRecognizerRef recognizer, void *context) { 
  Set_GraphText_layer_cgmsensor(graph_text_layer_cgmsensor, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_cgmsensor: Exiting###");
}

static void select_click_handler_cgmsensor(ClickRecognizerRef recognizer, void *context) {
    char * cgmchange = GetCGMSensorOption(INITIAL);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_cgmsensor - CGM Sensor: %s", cgmchange);
    snprintf(outputtext, sizeof(outputtext), "You are adding '%s'  to Care Portal.", cgmchange);
    snprintf(keyname, sizeof(keyname), "notes");
    snprintf(resultvalue, sizeof(resultvalue), "CGM Sensor: %s", cgmchange);
    snprintf(eventtype,sizeof(eventtype), "%s", cgmchange);
  
    create_populate_window();
}

static void down_click_handler_cgmsensor(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_cgmsensor(graph_text_layer_cgmsensor, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_cgmsensor: Exiting###");
}


static void click_config_provider_cgmsensor(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_cgmsensor);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_cgmsensor);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_cgmsensor);
}

void cgmsensor_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(cgmsensor_window);
#ifdef PBL_ROUND
  graph_text_layer_cgmsensor = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_cgmsensor = text_layer_create(GRect(0, 20, 144, 170));
#endif
 
  Set_GraphText_layer_cgmsensor(graph_text_layer_cgmsensor, INITIAL);
  text_layer_set_text_color(graph_text_layer_cgmsensor, COL_DARK);
  text_layer_set_background_color(graph_text_layer_cgmsensor, COL_LIGHT);
  text_layer_set_font(graph_text_layer_cgmsensor, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_cgmsensor, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_cgmsensor));
  
  window_set_click_config_provider(cgmsensor_window,(ClickConfigProvider)click_config_provider_cgmsensor);
}

void cgmsensor_unload_graph(Window *window) {
   if(graph_text_layer_cgmsensor)
   {
     text_layer_destroy(graph_text_layer_cgmsensor);
   }
   cgmsensorindex =0;
   window_destroy(cgmsensor_window);
}
/////////////////////// END OF CGM SENSOR ///////////////////////////////
//////// INSULIN CHANGE ////////////////////////////////////////////////////////////////
// returns the string of the new site location selection
char* GetInsulinChange(int change)
{
   insulinchangeindex = 0;// += change;
  
 /*  int count = sizeof(insulinchangeoptions)/sizeof(*insulinchangeoptions);
   app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###GetInsulinChange: count: %d ###", count);
   if(insulinchangeindex >= count)
   {
     insulinchangeindex = 0;
   }
   else if(insulinchangeindex < 0)
   {
     insulinchangeindex = count - 1;      
   } */
   snprintf(insulinchange_change, sizeof(insulinchange_change), "%s", insulinchangeoptions[insulinchangeindex]);
   return insulinchange_change;
}



void Set_GraphText_layer_insulinchange(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];

  char * insulinchange = GetInsulinChange(change);
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Insulin Option:\n %s", insulinchange);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "Insulin Option: %s", insulinchange);
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_insulinchange(ClickRecognizerRef recognizer, void *context) { 
  Set_GraphText_layer_insulinchange(graph_text_layer_insulinchange, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_insulinchange: Exiting###");
}

static void select_click_handler_insulinchange(ClickRecognizerRef recognizer, void *context) {
    char * insulinchange = GetInsulinChange(INITIAL);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_insulinchange - Insulin Option: %s", insulinchange);
    snprintf(outputtext, sizeof(outputtext), "You are adding \n '%s'  to Care Portal.", insulinchange);
    snprintf(keyname, sizeof(keyname), "notes");
    snprintf(resultvalue, sizeof(resultvalue), "Insulin Option: %s", insulinchange);
    snprintf(eventtype,sizeof(eventtype), "Insulin Change");
  
    create_populate_window();
}

static void down_click_handler_insulinchange(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_insulinchange(graph_text_layer_insulinchange, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_insulinchange: Exiting###");
}


static void click_config_provider_insulinchange(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_insulinchange);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_insulinchange);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_insulinchange);
}

void insulinchange_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(insulinchange_window);
#ifdef PBL_ROUND
  graph_text_layer_insulinchange = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_insulinchange = text_layer_create(GRect(0, 20, 144, 170));
#endif
 
  Set_GraphText_layer_insulinchange(graph_text_layer_insulinchange, INITIAL);
  text_layer_set_text_color(graph_text_layer_insulinchange, COL_DARK);
  text_layer_set_background_color(graph_text_layer_insulinchange, COL_LIGHT);
  text_layer_set_font(graph_text_layer_insulinchange, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_insulinchange, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_insulinchange));
  
  window_set_click_config_provider(insulinchange_window,(ClickConfigProvider)click_config_provider_insulinchange);
}

void insulinchange_unload_graph(Window *window) {
   if(graph_text_layer_insulinchange)
   {
     text_layer_destroy(graph_text_layer_insulinchange);
   }
   insulinchangeindex = 0;
   window_destroy(insulinchange_window);
}
/////////////////////// END OF INSULIN CHANGE ///////////////////////////////

//////// PROFILE SWITCH ////////////////////////////////////////////////////////////////
// returns the string of the new site location selection
/*char* GetProfileSwitch(int change)
{
   profileswitchindex += change;
  
   int count = sizeof(profileswitchoptions)/sizeof(*profileswitchoptions);
   app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###GetProfileSwitch: count: %d ###", count);
   if(profileswitchindex >= count)
   {
     profileswitchindex = 0;
   }
   else if(profileswitchindex < 0)
   {
     profileswitchindex = count - 1;      
   } 
   snprintf(profileswitch_change, sizeof(insulinchange_change), "%s", profileswitchoptions[profileswitchindex]);
   return profileswitch_change;
}

void Set_GraphText_layer_profileswitch(TextLayer* currentlayer, int change)
{
  static char s_packet_id_text[50];

  char * profileswitch = GetProfileSwitch(change);
  
  snprintf(s_packet_id_text, sizeof(s_packet_id_text), "Profile Switch:\n %s", profileswitch);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "Insulin Option: %s", profileswitch);
  text_layer_set_text(currentlayer, s_packet_id_text);
}

static void up_click_handler_profileswitch(ClickRecognizerRef recognizer, void *context) { 
  Set_GraphText_layer_profileswitch(graph_text_layer_profileswitch, UP);
	app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###up_click_handler_profileswitch: Exiting###");
}

static void select_click_handler_profileswitch(ClickRecognizerRef recognizer, void *context) {
    char * profileswitch = GetProfileSwitch(INITIAL);
    app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "select_click_handler_profileswitch - Profile Switch: %s", profileswitch);
    snprintf(outputtext, sizeof(outputtext), "You are adding Profile Switch:\n '%s'  to Care Portal.", profileswitch);
    snprintf(keyname, sizeof(keyname), "notes");
    snprintf(resultvalue, sizeof(resultvalue), "Profile Switch: %s", profileswitch);
    snprintf(eventtype,sizeof(eventtype), "Profile Switch");
    snprintf(profile,sizeof(profile), "%s", profileswitch);
    create_populate_window();
}

static void down_click_handler_profileswitch(ClickRecognizerRef recognizer, void *context) {
  Set_GraphText_layer_profileswitch(graph_text_layer_profileswitch, DOWN);
  app_log(APP_LOG_LEVEL_DEBUG, "main.c", 0, "###down_click_handler_profileswitch: Exiting###");
}


static void click_config_provider_profileswitch(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_profileswitch);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler_profileswitch);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_profileswitch);
}

void profileswitch_load_graph(Window *window) {
  
  ResetToDefaults();
  Layer *window_layer_graph = NULL;
  
  window_layer_graph = window_get_root_layer(profileswitch_window);
#ifdef PBL_ROUND
  graph_text_layer_profileswitch = text_layer_create(GRect(0, 60, 180, 170));
#else
  graph_text_layer_profileswitch = text_layer_create(GRect(0, 20, 144, 170));
#endif
 
  Set_GraphText_layer_profileswitch(graph_text_layer_profileswitch, INITIAL);
  text_layer_set_text_color(graph_text_layer_profileswitch, COL_DARK);
  text_layer_set_background_color(graph_text_layer_profileswitch, COL_LIGHT);
  text_layer_set_font(graph_text_layer_profileswitch, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(graph_text_layer_profileswitch, GTextAlignmentCenter);
  layer_add_child(window_layer_graph, text_layer_get_layer(graph_text_layer_profileswitch));
  
  window_set_click_config_provider(profileswitch_window,(ClickConfigProvider)click_config_provider_profileswitch);
}

void profileswitch_unload_graph(Window *window) {
   if(graph_text_layer_profileswitch)
   {
     text_layer_destroy(graph_text_layer_profileswitch);
   }
   insulinchangeindex =0;
   window_destroy(profileswitch_window);
}
*/
/////////////////////// END OF PROFILE SWITCH ///////////////////////////////


///////////////////////////// MAIN WINDOW /////////////////////////////////
static void menu_select_callback(int index, void *ctx) {
  
  if(index == 0)
  {
      carbs_insulin_window = window_create();
  	  window_set_window_handlers(carbs_insulin_window, 
  							   (WindowHandlers){
												.load   = carbs_insulin_load_graph,
											  .unload = carbs_insulin_unload_graph,
                                   }
  							   );  
  							  
  	  window_stack_push(carbs_insulin_window, true);
  }
  else if(index == 1)
  {
      carbs_window = window_create();
  	  window_set_window_handlers(carbs_window, 
  							   (WindowHandlers){
												.load   = carbs_load_graph,
											  .unload = carbs_unload_graph,
								   }
  							   );  
  							  
  	  window_stack_push(carbs_window, true);
  }
  else if(index == 2)
  {
        insulin_window = window_create();
    	  window_set_window_handlers(insulin_window, 
    							   (WindowHandlers){
													.load   = insulin_load_graph,
												  .unload = insulin_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(insulin_window, true);
  }
  else if(index == 3)
  {
        tempbasal_window = window_create();
    	  window_set_window_handlers(tempbasal_window, 
    							   (WindowHandlers){
													.load   = tempbasal_load_graph,
												  .unload = tempbasal_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(tempbasal_window, true);
  } 
  else if(index == 4)
  {
        bg_window = window_create();
    	  window_set_window_handlers(bg_window, 
    							   (WindowHandlers){
													.load   = bg_load_graph,
												  .unload = bg_unload_graph,
                                     }
    							   );  
    							  
    	  window_stack_push(bg_window, true);
  }
  else if(index == 5)
    {
        combobolus_window = window_create();
        window_set_window_handlers(combobolus_window, 
            (WindowHandlers){
                .load   = combobolus_load_graph,
                    .unload = combobolus_unload_graph,
        }
        );  

        window_stack_push(combobolus_window, true);

    }
  else if(index == 6)
  {
        pumpsitechange_window = window_create();
        window_set_window_handlers(pumpsitechange_window, 
            (WindowHandlers){
                .load   = pumpsitechange_load_graph,
                    .unload = pumpsitechange_unload_graph,
        }
        );  

        window_stack_push(pumpsitechange_window, true);
    }
  else if(index == 7)
  {
    exercise_window = window_create();
        window_set_window_handlers(exercise_window, 
            (WindowHandlers){
                .load   = exercise_load_graph,
                    .unload = exercise_unload_graph,
        }
        );  

        window_stack_push(exercise_window, true);
  }
  else if(index == 8)
  {
        cgmsensor_window = window_create();
        window_set_window_handlers(cgmsensor_window, 
            (WindowHandlers){
                .load   = cgmsensor_load_graph,
                    .unload = cgmsensor_unload_graph,
        }
        );  

        window_stack_push(cgmsensor_window, true);
    }  
   else if(index == 9)
   {
        insulinchange_window = window_create();
        window_set_window_handlers(insulinchange_window, 
            (WindowHandlers){
                .load   = insulinchange_load_graph,
                    .unload = insulinchange_unload_graph,
        }
        );  

        window_stack_push(insulinchange_window, true);
    }  
//   else if(index == 10)
//  {
//        profileswitch_window = window_create();
//        window_set_window_handlers(profileswitch_window, 
//            (WindowHandlers){
//                .load   = profileswitch_load_graph,
//                    .unload = profileswitch_unload_graph,
//        }
//       );  

//        window_stack_push(profileswitch_window, true);
//    }  
}

static void main_window_load(Window *window) {
    int num_a_items = 0;

	s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Carbs and Insulin",
            .callback = menu_select_callback,
    };
	
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Carbs",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Insulin",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Temp Basal",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Blood Glucose",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Combo Bolus",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Pump Site Change",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Exercise",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "CGM Sensor",
            .callback = menu_select_callback,
    };
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = "Insulin Change",
            .callback = menu_select_callback,
    };
//	s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
//        .title = "Profile Switch",
//            .callback = menu_select_callback,
//    };
	
   // Version information
    snprintf(version, sizeof(version), "Version %d.%d", __pbl_app_info.process_version.major, __pbl_app_info.process_version.minor);
    s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = version,
    };

    s_menu_sections[0] = (SimpleMenuSection) {
        .num_items = NUM_FIRST_MENU_ITEMS,
            .items = s_first_menu_items,
    };

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);

    s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

void main_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
  gbitmap_destroy(s_menu_icon_image);
}
////////////////////////////// END OF MAIN WINDOW //////////////////////////////////////////
static void init() 
{
    s_main_window = window_create();
  
    // Open AppMessage
   // app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    app_message_open(300, 300);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = main_window_load,
      .unload = main_window_unload,
    });
  
  
  // Registering callbacksGColorOxfordBlue
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
  
    window_stack_push(s_main_window, true);
  }

static void deinit() {
  window_destroy(s_main_window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "(free: %d, used: %d)",  heap_bytes_free(), heap_bytes_used());
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
