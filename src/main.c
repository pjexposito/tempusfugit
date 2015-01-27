#include "pebble.h"
  
#define COLOR_PRINCIPAL GColorWhite  // El color del lápiz es blanco
#define COLOR_FONDO GColorBlack  // y el fondo, negro

const unsigned short TAMANO[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 
                                    2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
                                    2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
                                    2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
                                    2, 4, 6, 8, 10, 12, 14, 16, 18, 20,
                                    2, 4, 6, 8, 10, 12, 14, 16, 18, 20
                                };   // Es la matriz de las posiciones de cada minuto
                                     // tengo que reducirla, ya que si se divide entre 10, se obtiene lo mismo

const unsigned short POSICION[] = {7, 29, 51, 75, 97, 119};  // Posición de cada grupo de 10 minutos
  
static Window *window;


//Capas del reloj
Layer *CapaLineas; // La capa principal donde se dibuja el reloj
TextLayer *CapaReloj; // Aquí pinto el reloj
TextLayer *CapaBateria; // Esto es la batería


//Esta función transforma la hora en una posición en la pantalla
int hora_a_posicion(int valor) 
{ 
  valor = -((6*valor)-151);
  return valor;
}


// Esta función se ejecutará cada vez que se refresque la CapaLineas
void CapaLineas_update_callback(Layer *me, GContext* ctx) 
{
  // Variables para conseguir la hora  
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    unsigned int minutos = t->tm_min;
    unsigned int hora = t->tm_hour;
  
  // Color del fondo y color del trazo
    graphics_context_set_stroke_color(ctx, COLOR_PRINCIPAL);
    graphics_context_set_fill_color(ctx, COLOR_PRINCIPAL);


  // Aquí se dibujan todas las líneas de las horas que han pasado ya
   for (unsigned int i=0;i<hora;i++)
    {
      for (int p=0;p<6;p++) 
       {
        graphics_fill_rect(ctx, GRect(POSICION[p], hora_a_posicion(i), 20, 4), 0, GCornerNone);  
       }  
    } 
  
  // Cuando están las lineas pintadas, se pintan los minutos  
   for (unsigned int i=1;i<minutos+1;i++)
    {
    graphics_fill_rect(ctx, GRect(i<2?POSICION[0]:POSICION[(i-1)/10]+(TAMANO[i]-2), hora_a_posicion(hora), 2, 4), 0, GCornerNone);
    }
  
}  // Y termina la función


void update_time_text() 
{  
  // Igual que antes, se definen las variables de tiempo
  static char timeText[] = "00:00";
  char *timeFormat;
  time_t now = time(NULL);
  const struct tm *currentTime = localtime(&now);

  
  // Según la configuración del reloj, se elije uno y otro formato
  if (clock_is_24h_style()) {
    timeFormat = "%R";
  } else {
    timeFormat = "%I:%M";
  }
  
  // Aquí hay dragones. Ni idea de lo que hace. Lo pillé de un tutorial
  strftime(timeText, sizeof(timeText), timeFormat, currentTime);
  if (!clock_is_24h_style() && (timeText[0] == '0')) {
    memmove(timeText, &timeText[1], sizeof(timeText) - 1);
  }

  // Y se pinta el texto en la capa de texto.
  text_layer_set_text(CapaReloj, timeText);

  // Y se destruye la capa para volver a construirla y pintarla de nuevo
  layer_mark_dirty(CapaLineas);
}


// Función que dibuja la batería en cada cambio de estado
static void handle_battery(BatteryChargeState charge_state) 
{
  static char battery_text[] = "Cargado";
  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "cargando");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(CapaBateria, battery_text);
}


// Aquí se añaden las funciones que ocurrirán en cada cambio de segundo
static void handle_minutos_tick(struct tm *tick_time, TimeUnits units_changed) 
{
  update_time_text();
  handle_battery(battery_state_service_peek());
}

static void init() 
{
  window = window_create();
  window_stack_push(window, true /* Animado */);
  window_set_background_color(window, COLOR_FONDO);
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //Se añade la CapaLineas
  CapaLineas = layer_create(bounds);
  layer_set_update_proc(CapaLineas, CapaLineas_update_callback); 
  layer_add_child(window_layer, CapaLineas); 
  
  //Se añade la CapaReloj
  CapaReloj = text_layer_create(GRect(0, 154, 72, 14));
  text_layer_set_text_alignment(CapaReloj, GTextAlignmentCenter);
  text_layer_set_text_color(CapaReloj, COLOR_PRINCIPAL);
  text_layer_set_background_color(CapaReloj, COLOR_FONDO);
  text_layer_set_font(CapaReloj, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, text_layer_get_layer(CapaReloj));
  
  //Se añade la CapaBatería
  CapaBateria = text_layer_create(GRect(72, 154, 70, 14));
  text_layer_set_text_color(CapaBateria, COLOR_PRINCIPAL);
  text_layer_set_background_color(CapaBateria, COLOR_FONDO);
  text_layer_set_font(CapaBateria, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(CapaBateria, GTextAlignmentCenter);
  text_layer_set_text(CapaBateria, "100% charged");
  layer_add_child(window_layer, text_layer_get_layer(CapaBateria));
  
  
  //Para terminar, activamos los servicios de cambio de minuto y de refresco de batería
  battery_state_service_subscribe(&handle_battery);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minutos_tick);  
  
}

static void deinit() 
{
 
  //Al cerrar la aplicación, matamos las capas y desactivamos los procesos
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  
  text_layer_destroy(CapaBateria);
  layer_destroy(CapaLineas);
  text_layer_destroy(CapaReloj);
  
  // Y por último se borra de la memoria la ventana principal
  window_destroy(window);
}

int main(void) 
{
  init();
  app_event_loop();
  deinit();
}
