using Gtk;

// FIXME: dummy C header is what's actually used, we need a glib-vala-mkenums
enum CheeseType {
	EDAM,
	STILTON,
	CHEDDAR
}

public class TestApp {
	public Gtk.CheckButton checkbox;
	public Gtk.Frame frame;

	/* FIXME: property binding support in Vala would be awesome */
	public void checkbox_active_notify (ParamSpec pspec) {
		frame.sensitive = checkbox.active;
	}

	public TestApp (GLib.Settings settings) {
		var window = new Window (WindowType.TOPLEVEL);
		window.title = "Gsettings test";
		window.border_width = 0;
		window.destroy.connect (Gtk.main_quit);

		checkbox = new CheckButton.with_label ("Enable");
		checkbox.notify["active"].connect (checkbox_active_notify);

		var entry = new Entry ();

		frame = new Frame (null);
		frame.border_width = 8;
		frame.add (entry);

		var box = new VBox (false, 0);
		box.pack_start (checkbox, true, true, 0);
		box.pack_start (frame, true, true, 0);

		window.add (box);

		settings.bind ("on", checkbox, "active", 0);
		settings.bind ("greeting", entry, "text", 0);

		window.show_all ();
	}
}
int main (string[] args) {
	Gtk.init (ref args);

	var settings = new GLib.Settings ("org.gsettings.test");

	var app = new TestApp (settings);

	Gtk.main();

	return 0;
}
