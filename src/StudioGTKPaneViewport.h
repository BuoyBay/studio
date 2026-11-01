/* Linux pane content must not impose GtkFixed's child requisition on GtkPaned.
   Studio lays out these children from the allocated pane size. A viewport keeps
   that allocation independent of an embedded WebKit view's natural minimum. */
#if defined(__linux__)
API int studio_gtk_clip_pane(void *content) {
    void *(*parent_of)(void *) = dlsym(RTLD_DEFAULT, "gtk_widget_get_parent");
    void *(*start_of)(void *) = dlsym(RTLD_DEFAULT, "gtk_paned_get_start_child");
    void *(*end_of)(void *) = dlsym(RTLD_DEFAULT, "gtk_paned_get_end_child");
    void (*set_start)(void *, void *) = dlsym(RTLD_DEFAULT, "gtk_paned_set_start_child");
    void (*set_end)(void *, void *) = dlsym(RTLD_DEFAULT, "gtk_paned_set_end_child");
    void *(*scroll_new)(void) = dlsym(RTLD_DEFAULT, "gtk_scrolled_window_new");
    void (*set_child)(void *, void *) = dlsym(RTLD_DEFAULT, "gtk_scrolled_window_set_child");
    void (*policy)(void *, int, int) = dlsym(RTLD_DEFAULT, "gtk_scrolled_window_set_policy");
    void (*expand_h)(void *, int) = dlsym(RTLD_DEFAULT, "gtk_widget_set_hexpand");
    void (*expand_v)(void *, int) = dlsym(RTLD_DEFAULT, "gtk_widget_set_vexpand");
    void *(*ref)(void *) = dlsym(RTLD_DEFAULT, "g_object_ref");
    void (*unref)(void *) = dlsym(RTLD_DEFAULT, "g_object_unref");
    if (!content || !parent_of || !start_of || !end_of || !set_start || !set_end ||
        !scroll_new || !set_child || !policy || !expand_h || !expand_v || !ref || !unref)
        return 0;
    void *parent = parent_of(content);
    if (!parent) return 0;
    int leading = start_of(parent) == content;
    if (!leading && end_of(parent) != content) return 0;
    void *scroll = scroll_new();
    if (!scroll) return 0;
    /* GTK_POLICY_EXTERNAL hides the scrollbars without propagating the child
       minimum, unlike GTK_POLICY_NEVER. Native editor controls own scrolling. */
    policy(scroll, 3, 3);
    expand_h(scroll, 1);
    expand_v(scroll, 1);
    ref(content);
    if (leading) set_start(parent, NULL); else set_end(parent, NULL);
    set_child(scroll, content);
    if (leading) set_start(parent, scroll); else set_end(parent, scroll);
    unref(content);
    return 1;
}
#endif
