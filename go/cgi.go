
package main

import (
	"fmt"
	"net/http"
	"strings"
)

func serveHttp(w http.ResponseWriter, r *http.Request) {
	r.ParseForm()
	if strings.HasPrefix(r.URL.Path, "/fetch") {
		fmt.Printf("%v\n", r.FormValue("url"))
	}
}

func main() {
	http.HandleFunc("/", serveHttp)
	err := http.ListenAndServe(":8082", nil)
	if err != nil {
		fmt.Printf("%v\n", err)
	}
}

