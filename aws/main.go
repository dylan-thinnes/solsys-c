package main

import (
	"github.com/aws/aws-lambda-go/lambda"
    "os/exec"
)
type BasicRequest struct {
    X string `json:"x"`
}

func lambda_factorize(event BasicRequest) (string, error) {
    return exec_factorize(event.X, []string{})
}

func exec_factorize(x string, flags []string) (string, error) {
    var args []string
    args = append(args, x)
    args = append(args, flags...)

    cmd := exec.Command("./solsys", args...)
    stdout, err := cmd.Output()
    if err != nil {
        return "", err;
    }
    return string(stdout), nil;
}

func main() {
	// Make the handler available for Remote Procedure Call by AWS Lambda
	lambda.Start(lambda_factorize)
}
