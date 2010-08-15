using Gtk;

// FIXME: dummy C header is what's actually used, we need a glib-vala-mkenums
enum CheeseType {
	EDAM,
	STILTON,
	CHEDDAR
}

int main (string[] args) {
	Gtk.init (ref args);

	var settings = new GLib.Settings ("org.gsettings.test");

	var window = new Window (WindowType.TOPLEVEL);
	window.title = "Gsettings test";
	window.destroy.connect (Gtk.main_quit);

	var checkbox = new CheckButton.with_label ("Enable");
	window.add (checkbox);

	window.show_all ();

	Gtk.main();
	return 0;
}
