package main

import (
	"flag"
	"fmt"
	"log"
	"os/exec"
)

var (
	problem   = flag.String("problem", "LA001", `problem name e.g. "LA001"`)
	solver    = flag.String("solver", "//solver:simple_solve", "ai target")
	skipSetup = flag.Bool("skip-setup", true, "specify to skip setup file")
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

func setupFiles() {
	log.Print("Setup files")
	runCommands("gsutil -m rsync -r gs://negainoido-icfpc2018-shared-bucket/problemsL shared")
	runCommands("gsutil -m rsync -r gs://negainoido-icfpc2018-shared-bucket/dfltTracesL shared")
}

func main() {
	flag.Parse()

	if !*skipSetup {
		setupFiles()
	}

	runCommands(fmt.Sprintf("bazel run %s -- --mdl_filename=$(pwd)/shared/%s_tgt.mdl > /tmp/nbt.nbt", *solver, *problem))
	simulatorResult := runCommandsOutput(fmt.Sprintf("bazel run //src:simulator -- --mdl_filename=$(pwd)/shared/%s_tgt.mdl --nbt_filename=/tmp/nbt.nbt", *problem))

	log.Printf("Simulator Result %s", simulatorResult)

	officialResult := runCommandsOutput(fmt.Sprintf("python3 soren/main.py $(pwd)/shared/%s_tgt.mdl /tmp/nbt.nbt", *problem))

	log.Printf("Official Result %s", officialResult)

	if simulatorResult != officialResult {
		log.Fatalf("Different! simulator %s vs official %s", simulatorResult, officialResult)
	}

	log.Print("OK!")
}
