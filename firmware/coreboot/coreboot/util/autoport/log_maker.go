package main

import (
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"strings"
)

func TryRunAndSave(output string, name string, arg []string) error {
	cmd := exec.Command(name, arg...)

	f, err := os.Create(output)
	if err != nil {
		log.Fatal(err)
	}

	cmd.Stdout = f
	cmd.Stderr = f

	err = cmd.Start()
	if err != nil {
		return err
	}
	cmd.Wait()
	return nil
}

func RunAndSave(output string, name string, arg ...string) {
	err := TryRunAndSave(output, name, arg)
	if err == nil {
		return
	}
	idx := strings.LastIndex(name, "/")
	relname := name
	if idx >= 0 {
		relname = name[idx+1:]
	}
	relname = "./" + relname
	err = TryRunAndSave(output, relname, arg)
	if err != nil {
		log.Fatal(err)
	}
}

const MAXPROMPTRETRY = 3

func PromptUser(prompt string, opts []string) (match string, err error) {
	for i := 1; i < MAXPROMPTRETRY; i++ {
		fmt.Printf("%s. (%s) Default:%s\n", prompt,
		           strings.Join(opts, "/"), opts[0])
		var usrInput string
		fmt.Scanln(&usrInput)

		// Check for default entry
		if usrInput == "" {
			match = opts[0]
			return
		}

		for _, opt := range opts {
			if opt == usrInput {
				match = opt
				return
			}
		}
	}
	err = errors.New("max retries exceeded")
	fmt.Fprintln(os.Stderr, "ERROR: max retries exceeded")
	return
}

func MakeLogs(outDir string) {
	os.MkdirAll(outDir, 0700)
	RunAndSave(outDir+"/lspci.log", "lspci", "-nnvvvxxxx")
	RunAndSave(outDir+"/dmidecode.log", "dmidecode")
	RunAndSave(outDir+"/acpidump.log", "acpidump")

	inteltoolArgs := "-a"
	opt, err := PromptUser("WARNING: The following tool MAY cause your system to hang when it attempts "+
		"to probe for graphics registers.  Having the graphics registers will help create a better port. "+
		"Should autoport probe these registers?",
		[]string{"y", "yes", "n", "no"})

	// Continue even if there is an error

	switch opt {
	case "y", "yes":
		inteltoolArgs += "f"
	}

	RunAndSave(outDir+"/inteltool.log", "../inteltool/inteltool", inteltoolArgs)
	RunAndSave(outDir+"/ectool.log", "../ectool/ectool", "-pd")
	RunAndSave(outDir+"/superiotool.log", "../superiotool/superiotool", "-ade")

	SysDir := "/sys/class/sound/card0/"
	files, _ := ioutil.ReadDir(SysDir)
	for _, f := range files {
		if (strings.HasPrefix(f.Name(), "hw") || strings.HasPrefix(f.Name(), "hdaudio")) && f.IsDir() {
			in, err := os.Open(SysDir + f.Name() + "/init_pin_configs")
			defer in.Close()
			if err != nil {
				log.Fatal(err)
			}
			out, err := os.Create(outDir + "/pin_" + strings.Replace(f.Name(), "hdaudio", "hw", -1))
			if err != nil {
				log.Fatal(err)
			}
			defer out.Close()
			io.Copy(out, in)
		}
	}

	ProcDir := "/proc/asound/card0/"
	files, _ = ioutil.ReadDir(ProcDir)
	for _, f := range files {
		if strings.HasPrefix(f.Name(), "codec#") && !f.IsDir() {
			in, err := os.Open(ProcDir + f.Name())
			defer in.Close()
			if err != nil {
				log.Fatal(err)
			}
			out, err := os.Create(outDir + "/" + f.Name())
			if err != nil {
				log.Fatal(err)
			}
			defer out.Close()
			io.Copy(out, in)
		}
	}

	for _, fname := range []string{"cpuinfo", "ioports"} {
		in, err := os.Open("/proc/" + fname)
		defer in.Close()
		if err != nil {
			log.Fatal(err)
		}
		out, err := os.Create(outDir + "/" + fname + ".log")
		if err != nil {
			log.Fatal(err)
		}
		defer out.Close()
		io.Copy(out, in)
	}

	out, err := os.Create(outDir + "/input_bustypes.log")
	if err != nil {
		log.Fatal(err)
	}
	defer out.Close()

	ClassInputDir := "/sys/class/input/"
	files, _ = ioutil.ReadDir(ClassInputDir)
	for _, f := range files {
		if strings.HasPrefix(f.Name(), "input") && !f.Mode().IsRegular() { /* Allow both dirs and symlinks.  */
			in, err := os.Open(ClassInputDir + f.Name() + "/id/bustype")
			defer in.Close()
			if err != nil {
				log.Fatal(err)
			}
			io.Copy(out, in)
		}
	}
}
