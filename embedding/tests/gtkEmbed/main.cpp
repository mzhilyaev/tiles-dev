
//
// What follows is a hacked version the the sample hello
// world gtk app found: http://www.gtk.org/tutorial/gtk_tut-2.html
//


#include <gtk/gtk.h>
 
#include "nsCOMPtr.h"
#include "nsEmbedAPI.h"
#include "WebBrowserChrome.h"

#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

static gint  io_identifier = 0;

gint delete_window( GtkWidget *widget,
                    GdkEvent  *event,
                    gpointer   data )
{
    gtk_main_quit();
    return(FALSE);
}

void
url_activate_cb( GtkEditable *widget, nsIWebBrowserChrome *browser)
{
  gchar *text = gtk_editable_get_chars(widget, 0, -1);
  g_print("loading url %s\n", text);
  if (browser)
  {
      nsCOMPtr<nsIWebBrowser> webBrowser;
      browser->GetWebBrowser(getter_AddRefs(webBrowser));
      nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(webBrowser));
      if (webNav)
	  webNav->LoadURI(NS_ConvertASCIItoUCS2(text).GetUnicode());

  }       
  g_free(text);
}

static void
handle_event_queue(gpointer data, gint source, GdkInputCondition condition)
{
  nsIEventQueue *eventQueue = (nsIEventQueue *)data;
  eventQueue->ProcessPendingEvents();
}


nsresult OpenWebPage(char* url)
{
    WebBrowserChrome * chrome = new WebBrowserChrome();
    if (!chrome)
        return NS_ERROR_FAILURE;
    
    NS_ADDREF(chrome); // native window will hold the addref.

    nsCOMPtr<nsIWebBrowser> newBrowser;
    chrome->CreateBrowserWindow(0, getter_AddRefs(newBrowser));
    if (!newBrowser)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(newBrowser);
    
    baseWindow->SetPositionAndSize(0, 
                                   0, 
                                   430, 
                                   430,
                                   PR_TRUE);

    baseWindow->SetVisibility(PR_TRUE);

    GtkWidget *widget;
    baseWindow->GetParentNativeWindow((void**)&widget);
 

    gtk_widget_show (widget); // should this function do this?

    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(newBrowser));

    if (!webNav)
	  return NS_ERROR_FAILURE;

    return webNav->LoadURI(NS_ConvertASCIItoUCS2(url).GetUnicode());
}   



nativeWindow CreateNativeWindow(nsIWebBrowserChrome* chrome)
{
   GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   GtkWidget *urlEntry = gtk_entry_new();
   GtkWidget *box = gtk_vbox_new(FALSE, 0);
   GtkWidget *view = gtk_event_box_new();

   if (!window || !urlEntry || !box || !view)
     return nsnull;

   /* Put the box into the main window. */
   gtk_container_add (GTK_CONTAINER (window), box);

   gtk_box_pack_start(GTK_BOX(box), urlEntry, FALSE, FALSE, 5);
   gtk_box_pack_start(GTK_BOX(box), view,     FALSE, FALSE, 5);

   gtk_signal_connect (GTK_OBJECT (window), 
                       "delete_event",
                        GTK_SIGNAL_FUNC (delete_window), 
                        chrome);

   gtk_signal_connect(GTK_OBJECT(urlEntry), "activate",
		     GTK_SIGNAL_FUNC(url_activate_cb), 
                     chrome);

   gtk_widget_set_usize(view, 430, 430);

   gtk_window_set_title (GTK_WINDOW (window), "Embedding is Fun!");
   gtk_window_set_default_size (GTK_WINDOW(window), 450, 450);

   gtk_container_set_border_width (GTK_CONTAINER (window), 10);

   gtk_widget_realize (window);

   gtk_widget_show (urlEntry);
   gtk_widget_show (box);
   gtk_widget_show (view);
   gtk_widget_show (window);

  return view;
}



int main( int   argc,
		  char *argv[] )
{
  nsresult rv;
  NS_InitEmbedding(nsnull, nsnull);

  // set up the thread event queue
  nsIEventQueueService* eventQService;
  rv = nsServiceManager::GetService(kEventQueueServiceCID,
									NS_GET_IID(nsIEventQueueService),
									(nsISupports **)&eventQService);
  if (NS_OK == rv)
	{
	  // get our hands on the thread event queue
	  nsIEventQueue *eventQueue;
	  rv = eventQService->GetThreadEventQueue( NS_CURRENT_THREAD, 
											   &eventQueue);
	  if (NS_FAILED(rv))
		return FALSE;
    
	  io_identifier = gdk_input_add( eventQueue->GetEventQueueSelectFD(),
									 GDK_INPUT_READ,
									 handle_event_queue,
									 eventQueue);
	  NS_RELEASE(eventQService);
	  NS_RELEASE(eventQueue);
	}

  /* This is called in all GTK applications. Arguments are parsed
   * from the command line and are returned to the application. */
  gtk_init(&argc, &argv);
         

  if ( NS_FAILED( OpenWebPage("http://people.netscape.com/dougt") ))
	  return (-1);
  
 
  /* All GTK applications must have a gtk_main(). Control ends here
   * and waits for an event to occur (like a key press or
   * mouse event). */
  gtk_main ();
         
  return(0);
}

