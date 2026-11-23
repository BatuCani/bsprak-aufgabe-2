#ifndef LIB_LIST_H_
#define LIB_LIST_H_
/*
 * \file list.h
 * \brief Doppelt verkettete List Datenstruktur
 *
 * Diese Datei ist eine Hilfestellung für Aufgabe 3.
 * Sie erspart euch den Aufwand, selber eine doppelt verkettete Liste zu implementieren.
 */

// List Node struct
typedef struct list_node {
	struct list_node *next;
	struct list_node *prev;
} list_node;

struct list {
	list_node *first;
	list_node *last;
};

// Makro zum initialisieren einer Liste
#define list_create(N)                                        \
	static struct list  head__##N = { nullptr, nullptr }; \
	static struct list *N	      = &(head__##N)

//checks if list is empty
[[nodiscard]] static inline bool list_is_empty(struct list *list)
{
	return list->first == nullptr;
}

// Füge Element am Anfang der Liste hinzu
[[maybe_unused]] static inline void list_add_first(struct list *list, list_node *new)
{
	new->next = list->first;
	new->prev = nullptr;
	if (new->next != nullptr) {
		new->next->prev = new;
	}

	list->first = new;
	if (list->last == nullptr) {
		list->last = new;
	}
}

// Füge Element am Ende der Liste hinzu
[[maybe_unused]] static inline void list_add_last(struct list *list, list_node *new)
{
	new->prev = list->last;
	new->next = nullptr;
	if (new->prev != nullptr) {
		new->prev->next = new;
	}

	list->last = new;
	if (list->first == nullptr) {
		list->first = new;
	}
}

// Entfernt das erste Element aus der Liste und gibt es zurück
[[maybe_unused]] static inline list_node *list_remove_first(struct list *list)
{
	list_node *del = list->first;
	if (del == nullptr) {
		return nullptr;
	}

	list->first = del->next;
	if (list->last == del) {
		list->last = nullptr;
	}
	if (del->next) {
		del->next->prev = nullptr;
		del->next	= nullptr;
	}

	return del;
}

// Entferne das letzte Element aus der Liste und gibt es zurück
[[maybe_unused]] static inline list_node *list_remove_last(struct list *list)
{
	list_node *del = list->last;
	if (del == nullptr) {
		return nullptr;
	}

	list->last = del->prev;
	if (list->first == del) {
		list->first = nullptr;
	}
	if (del->prev) {
		del->prev->next = nullptr;
		del->prev	= nullptr;
	}

	return del;
}

// Entfernt das spezielle Element aus der Liste
[[maybe_unused]] static inline list_node *list_remove(struct list *list, list_node *del)
{
	if (del == list->first) {
		return list_remove_first(list);
	}
	if (del == list->last) {
		return list_remove_last(list);
	}

	del->prev->next = del->next;
	del->next->prev = del->prev;

	del->next = nullptr;
	del->prev = nullptr;
	return del;
}

[[maybe_unused]] static inline unsigned int list_length(struct list *list)
{
	unsigned int len  = 0;
	list_node   *node = list->first;
	while (node != nullptr) {
		node = node->next;
		len++;
	}
	return len;
}
#endif // LIB_LIST_H_
