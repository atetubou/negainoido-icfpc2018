package main

import (
	"flag"
	"fmt"
	"log"
	"os/exec"
	"strings"
)

var (
	problem = flag.String("problem", "FA001", `problem name e.g. "FA001"`)
	solver  = flag.String("solver", "//solver:simple_solve", "ai target")
)

func runCommands(command string) {
	err := exec.Command("bash", "-c", command).Run()
	if err != nil {
		log.Fatal(command, err)
	}
}

func runCommandsOutput(command string) string {
	output, err := exec.Command("bash", "-c", command).Output()
	if err != nil {
		log.Fatal(command, err)
	}
	return string(output)
}

func main() {
	flag.Parse()

	srcMDL := fmt.Sprintf("$(pwd)/shared/%s_src.mdl", *problem)
	tgtMDL := fmt.Sprintf("$(pwd)/shared/%s_tgt.mdl", *problem)

	if strings.HasPrefix("LA", *problem) || strings.HasPrefix("FA", *problem) {
		srcMDL = "-"
	}

	if strings.HasPrefix("FD", *problem) {
		tgtMDL = "-"
	}

	solveCommand := fmt.Sprintf("bazel run %s -- --mdl_filename=%s > /tmp/nbt.nbt", *solver, tgtMDL)
	runCommands()
	simulatorResult := runCommandsOutput(fmt.Sprintf("bazel run //src:simulator -- --mdl_filename=%s --nbt_filename=/tmp/nbt.nbt", tgtMDL))

	log.Printf("Simulator Result %s", simulatorResult)

	officialResult := runCommandsOutput(fmt.Sprintf("python3 soren/main.py %s %s /tmp/nbt.nbt", srcMDL, tgtMDL))

	log.Printf("Official Result %s", officialResult)

	if simulatorResult != officialResult {
		log.Fatalf("Different! simulator %s vs official %s", simulatorResult, officialResult)
	}

	log.Print("OK!")
}
