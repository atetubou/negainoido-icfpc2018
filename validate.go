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

	tgtFilename = flag.String("tgt_filename", "", "tgt filanem")
	srcFilename = flag.String("src_filename", "", "src filanem")
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

	if strings.HasPrefix(*problem, "LA") || strings.HasPrefix(*problem, "FA") {
		srcMDL = "-"
	}

	if strings.HasPrefix(*problem, "FD") {
		tgtMDL = "-"
	}

	if *srcFilename != "" {
		srcMDL = *srcFilename
	}

	if *tgtFilename != "" {
		tgtMDL = *tgtFilename
	}

	// TODO: use tmpdir
	runCommands(fmt.Sprintf("bazel run %s -- --src_filename %s --tgt_filename %s > /tmp/nbt.nbt", *solver, srcMDL, tgtMDL))
	simulatorResult := runCommandsOutput(fmt.Sprintf("bazel run //src:simulator -- --src_filename=%s --tgt_filename=%s --nbt_filename=/tmp/nbt.nbt",
		srcMDL, tgtMDL))

	log.Printf("Simulator Result %s", simulatorResult)

	officialResult := runCommandsOutput(fmt.Sprintf("python3 soren/main.py %s %s /tmp/nbt.nbt", srcMDL, tgtMDL))

	log.Printf("Official Result %s", officialResult)

	if simulatorResult != officialResult {
		log.Fatalf("Different! simulator %s vs official %s", simulatorResult, officialResult)
	}

	log.Print("OK!")
}
