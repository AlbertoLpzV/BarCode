/* 

RAÚL LINIO ALONSO 100346329

ALBERTO LÓPEZ VALERO 100346384


Funcionalidades implementadas:

- Funcionalidad básica
- Mejora: texto superpuesto
- Mejora: borde

*/

#include <gst/gst.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GST_TIME_FORMAT "u:%02u:%02u.%09u"   /* Definición tipo formato temporal */

GstElement *textoverlay, *videobox; /* Variables Globales */

gboolean func_intervalo = FALSE;  /* Flag activación funciones */
gboolean func_overlaytext = FALSE;
gboolean func_videobox = FALSE;

guint64 id; /* id cuenta atrás */


/*--------------------------------------------------------------------------------*/

static int * get_barcode(GstMessage *msg){  /* Función que  obtiene codigo zbar */

  const GstStructure *structure;
  const gchar *barcode;
  guint64 timestamp;

  structure = gst_message_get_structure (msg);  /* Obtenemos estructura del GstMessage */
  barcode = gst_structure_get_string (structure, "symbol");  /* Extraemos el código detectado por zbar */
  gst_structure_get (structure, "timestamp", G_TYPE_UINT64, &timestamp, NULL); /* Extraemos el instante donde se detecta código */

  g_print ("\nBarcode: %s [%"GST_TIME_FORMAT"]\n", barcode, GST_TIME_ARGS(timestamp));  /* Imprimimos */

  if (func_overlaytext == TRUE){  /* Si flag función overlaytext activado ... */
    g_object_set(G_OBJECT(textoverlay), "text", barcode, NULL);  /* Introducimos el texto a superponer a la funcion */
  }

  if (func_videobox == TRUE){  /* Si flag función videobox activado ... */
    g_object_set(G_OBJECT(videobox), "bottom", -10, NULL);  /* Damos valor a los bordes de la funcion videobox */
    g_object_set(G_OBJECT(videobox), "left", -10, NULL);
    g_object_set(G_OBJECT(videobox), "right", -10, NULL);
    g_object_set(G_OBJECT(videobox), "top", -10, NULL);
  }

  return 0;
}

/*--------------------------------------------------------------------------------*/

static void eraseText(){  /* Función que elimina efectos introducidos */
  
  if (func_overlaytext == TRUE){  /* Si flag función overlaytext activado ... */
    const gchar *clear_text = "";
    g_object_set(G_OBJECT(textoverlay), "text", clear_text, NULL);   /* Introducimos funcion texto vacío */
  }

  if (func_videobox == TRUE){  /* Si flag función videobox activado ... */
    g_object_set(G_OBJECT(videobox), "bottom", 0, NULL);  /* Damos valor 0 a los bordes de la funcion videobox */
    g_object_set(G_OBJECT(videobox), "left", 0, NULL);
    g_object_set(G_OBJECT(videobox), "right", 0, NULL);
    g_object_set(G_OBJECT(videobox), "top", 0, NULL);
  }

  g_source_remove( id ); /* Desactivo cuenta atrás para que no salte continuamente */

}

/*--------------------------------------------------------------------------------*/

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data ){  /* Handler de mensajes del bus */

  GMainLoop *loop = (GMainLoop *) data;
  
  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:  /* Si End of Stream ... */
      // g_print ("End of stream\n"); 
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: { /* Si mensaje de error ... */
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("\n\nError: error en el procesamiento multimedia\n\n");
      g_error_free (error);

      g_main_loop_quit (loop);
      return -1;
      break;
     }

     case GST_MESSAGE_ELEMENT:  /* Si mensaje de elemento ...  */
       
	if( strcmp(GST_MESSAGE_SRC_NAME(msg),"zbar")==0 ) { /* Si el mensaje de elemento es de zbar */
           
           get_barcode(msg);   /* Llamamos a función que extrae el codigo de zbar y lo gestiona */
           id = g_timeout_add(3000, (GSourceFunc) eraseText, NULL); /* Establecemos tiempo de visualizacion de efecto hasta su eliminación */

        }
      break;

     default:
      break;
  }

  return TRUE;
}

/*--------------------------------------------------------------------------------*/

static void on_pad_added (GstElement *element,   /* Quien genera evento */
              GstPad     *pad,       /* Pad que quiere conectarse */
              gpointer    data)     
{
  GstPad *sinkpad;

  GstElement *decoder = (GstElement *) data;

  sinkpad = gst_element_get_static_pad (decoder, "sink");  /* Pad decodificador */

  gst_pad_link (pad, sinkpad);  /* Link */

  gst_object_unref (sinkpad); /* Desreferencio cuando no utilizo */
}

/*--------------------------------------------------------------------------------*/

static void change_value_func_overlaytext(){  /* Activación flag función texto superpuesto */
  func_overlaytext = TRUE;
}

/*--------------------------------------------------------------------------------*/

static void change_value_func_videobox(){   /* Activación flag función borde video */
  func_videobox = TRUE;
}

/*--------------------------------------------------------------------------------*/

int main (int argc, char *argv[], char* argum[]){  /* Main */

  GMainLoop *loop;    /* Puntero a bucle de eventos*/

  GstElement *pipeline, *source, *decoder, *videoqueue, *conv_video, *sink_video, *audioqueue, *conv_audio, *sink_audio, *zbar;   /* Elementos que usaremos en nuestro pipeline*/
  
  GstBus *bus;  // Bus
  
  gint64 duration;  // Para guardar valor duracion pipeline
  
  /* Inicialización */
  gst_init (&argc, &argv);  /* Iniciamos con los valores pasados por el main */

  loop = g_main_loop_new (NULL, FALSE);  /* Inicializo el puntero con un nuevo loop */
  
  /* Comprobación argumentos de entrada*/
  if (argc < 2) {
    g_printerr ("\n\nUsar al menos un argumento: %s fichero_entrada [-h] [-i inicio] [-f fin] [-t] [-b] \n\n", argv[0]);  /* Imprimo error */
    return -1;
  }

  int cnt=2; /* Índice inicializado a 2 para saltar primeros 2 argumentos */
  char *argumento;

  while (cnt<argc){
     
     argumento = argv[cnt];

     if( strcmp(argumento, "-h") == 0 ){   /* Si activo funcionalidad ayuda */
         g_print("\n\nAyuda:\n\n"
                "./barcode  fichero_entrada [-h] [-i inicio] [-f fin ] [-t] [-b]\n\n"
                "Argumentos del programa:\n"
                "fichero_entrada: fichero de video a analizar\n"
                "-h: presenta la ayuda y termina (con estado 0)\n"
                "-i inicio: instante inicial a partir del cual reproducir y realizar la deteccion (en segundos)\n"
                "-f fin: instante final en el que detener la reproduccion y deteccion (en segundos)\n"
                "-t: superpone un texto con el numero de codigo de barras y su simbolo en la imagen visualizada  \n"
                "-b: aplica un borde de color rojo al video para resaltar las imagenes en que aparece un codigo de barras cambio de escena\n\n\n");
        
         return 0;
     }
     else if( strcmp(argumento, "-i") == 0 ){  /* Si activo funcionalidad intervalos de video */
        g_print("\n\nFuncionalidad [Intervalos de tiempo] no implementada \n\n");
     }     
     else if( strcmp(argumento, "-f") == 0 ){
        g_print("\n\nFuncionalidad [Intervalos de tiempo] no implementada \n\n");
     }     
     else if( strcmp(argumento, "-t") == 0 ){ /* Si activo funcionalidad texto superpuesto */
        change_value_func_overlaytext();  // Flag ejecucion texto superpuesto
     }     
     else if( strcmp(argumento, "-b") == 0 ){ /* Si activo funcionalidad bordes video */
        change_value_func_videobox();  // Flag ejeccucion marco rojo
     }else {                                                               /* Si paso por parámetro argumento inválido*/
        g_printerr("\nError: argumento[%s] no valido\n\n", argumento);  
        return 1;
     }
  cnt++;
  }
    
  
  /* Creo elementos gstreamer */
  pipeline = gst_pipeline_new ("audio-video-player"); /* Objeto pipeline */
  source   = gst_element_factory_make ("filesrc",       "file-source"); /* Creo elementos con funcion factoria (tipo elemento, nombre) */
  
  decoder  = gst_element_factory_make ("decodebin",     "dec");

  /* Zbar */
  zbar = gst_element_factory_make ("zbar", "zbar");  /* Creo elemento tipo Zbar llamado zbar */
  g_object_set (zbar, "cache", TRUE, NULL);  /* Activo cache del elemento zbar*/

  /* Text Overlay */
  textoverlay = gst_element_factory_make("textoverlay", "textoverlay");  /* Creo elemento tipo TextOverlay llamado textoverlay */
  g_object_set(G_OBJECT(textoverlay), "color", 0x000000, NULL);  /* Ajusto color texto */
  g_object_set(G_OBJECT(textoverlay), "font-desc", "Sans, 35", NULL);  /* Ajusto fuente y tamaño */

 /* Videobox */
  typedef enum {  /* Clase enum GstVideoBoxFill */

    VIDEO_BOX_FILL_BLACK,   // Colores
    VIDEO_BOX_FILL_GREEN,
    VIDEO_BOX_FILL_BLUE,
    VIDEO_BOX_FILL_RED,
    VIDEO_BOX_FILL_YELLOW,
    VIDEO_BOX_FILL_WHITE,
    VIDEO_BOX_FILL_LAST

  }GstVideoBoxFill;
  GstVideoBoxFill color = VIDEO_BOX_FILL_RED;  // Selecciono color

  videobox = gst_element_factory_make ("videobox", "videobox"); /* Creo elemento tipo VideoBox llamado videobox */
  g_object_set(G_OBJECT(videobox), "fill", color, NULL); /* Ajusto color */

  /* Elementos video */
  videoqueue = gst_element_factory_make ("queue",       "videoqueue");
  conv_video = gst_element_factory_make ("videoconvert","convertervideo");
  sink_video = gst_element_factory_make ("ximagesink",  "video-output");

  /* Elementos audio */
  audioqueue = gst_element_factory_make ("queue",       "audioqueue");
  conv_audio = gst_element_factory_make ("audioconvert","converteraudio");
  sink_audio = gst_element_factory_make ("autoaudiosink", "audio-output");


  if (!pipeline || !source || !decoder || !videoqueue|| !conv_video || !sink_video || !audioqueue || !conv_audio || !sink_audio || !zbar || !textoverlay || !videobox ) {  /* Comprobacion de que elementos se han creado */
    g_printerr ("\n\nError: error en el procesamiento multimedia.\n\n");  /* Imprimo error */
    return -1;
  }


  /* Construyo pipeline */

  /* Introducimos input filename al elemento source */
  g_object_set (G_OBJECT (source), "location", argv[1], NULL); 

  /* Añadimos manejador de eventos */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Añadimos elementos al pipeline */

  /* file-source | decodebin | queue | zbar | videobox | textoverlay | videoconvert | ximagesink */
  /* file-source | decodebin | queue | audioconvert | autoaudiosink */

  gst_bin_add_many (GST_BIN (pipeline),
                    source, decoder, videoqueue, zbar, videobox, textoverlay, conv_video, sink_video, audioqueue, conv_audio, sink_audio, NULL);  /* Añade lista de elementos al bin */

  /* Unimos elementos entre ellos */
  gst_element_link (source, decoder);

  /* file-source -> decodebin -> queue -> zbar -> videobox -> textoverlay -> videoconvert -> ximagesink */
  gst_element_link_many (videoqueue, zbar, videobox, textoverlay, conv_video, sink_video, NULL); /* Para pad estatico */
  g_signal_connect (decoder, "pad-added", G_CALLBACK (on_pad_added), videoqueue);  /* Genera un escuchador que invoca a la funcion cuando se active un pad */

  /* file-source -> decodebin -> queue -> audioconvert -> autoaudiosink */
  gst_element_link_many (audioqueue, conv_audio, sink_audio, NULL); /* Para pad estatico */
  g_signal_connect (decoder, "pad-added", G_CALLBACK (on_pad_added), audioqueue);  /* Genera un escuchador que invoca a la funcion cuando se active un pad */ 


/* En este punto ya tengo el pipeline listo */
  
  gst_element_set_state (pipeline, GST_STATE_PLAYING);  /* Ponemos el pipeline en estado "playing" */

  g_main_loop_run (loop);  /* Activamos loop */

  /* Para determinar duración pipeline */    /* Funcionalidad opertiva pero no activada por no tener el resto de la funcionalidad */
  if(func_intervalo == TRUE){  
    if (gst_element_query_duration (pipeline, GST_FORMAT_TIME, &duration)){
      g_print ("Duration = %"GST_TIME_FORMAT"\n", GST_TIME_ARGS(duration));
    } else {
      return 0;
    }
  }


  /* Cuando EOS ... */
  gst_element_set_state (pipeline, GST_STATE_NULL);  /* Ponemos el pipeline en estado "NULL" */
  gst_object_unref (bus); /* Desreferencio */
  gst_object_unref (GST_OBJECT (pipeline));  /* Desreferencio */
  g_main_loop_unref (loop); /* Desreferencio */

  return 0;
}



