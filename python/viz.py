#!/usr/bin/env python
import argparse
import csv
import os
import plotly
import plotly.graph_objs as go
from plotly.offline import plot

YAXIS_LABEL = {
    "throughput": "MB/sec",
    "wall-clock": "MS"
}

TITLES = {
    "throughput": "Throughput as a function of LUNs",
    "wall-clock": "Wall-Clock as a function of LUNs"
}

def plot_scale(topics, args):
    """Bar-chart showing throughput in MB/sec as a function of LUNs"""

    LUNS = range(1, 1024+1)

    for topic in topics["__META__"]["ORDER"]:
        if len(topics[topic]["luns"]) < len(LUNS):
            LUNS = topics[topic]["luns"]
    data = []

    glb_max = 0.0
    glb_min = 10000.0
    for topic in topics["__META__"]["ORDER"]:
        glb_max = max(glb_max, max(topics[topic][args.yaxis]))
        glb_min = min(glb_min, min(topics[topic][args.yaxis]))
        data.append(
            go.Scatter(
                x=LUNS,
                y=topics[topic][args.yaxis][0:len(LUNS)],
                name=topic.capitalize(),
                mode="lines"
            )
        )

    layout = go.Layout(
        title=TITLES[args.yaxis],
        barmode='group',
        xaxis=dict(
            title="#LUNs",
            tickangle=-45,
            zeroline=True,
            showgrid=True,
            showline=True,
            autorange=False,
            range=[0, max(LUNS) + 1]
        ),
        yaxis=dict(
            title=YAXIS_LABEL[args.yaxis],
            tickformat=".2f",
            zeroline=True,
            showgrid=True,
            showline=True,
            autorange=False,
            range=[0, glb_max + (glb_max / len(LUNS))]
        ),
    )

    fig = go.Figure(data=data, layout=layout)

    rpath = "/tmp"
    if args.output:
        rpath  = args.output

    fname = "%s-%s" % (args.yaxis, "-".join(topics["__META__"]["ORDER"]))
    fname = os.sep.join([rpath, fname])

    html_name = "%s.html" % fname
    image_name = "%s.png" % fname

    plot(
        fig,
        auto_open=False,
        filename=html_name,
        image="png",
        image_width=1600,
        image_height=1200,
        image_filename=image_name
    )

def main(args):

    topics = {"__META__": {"ORDER": []}}

    for res in args.result:
        path = os.path.abspath(os.path.expandvars(os.path.expanduser(res)))
        bname, _ = os.path.splitext(os.path.basename(path))

        if not os.path.exists(path):
            print("Invalid result")
            return

        if bname in topics:
            print("Duplicate result files")
            return

        topics[bname] = {
            "path": path,
            "results": []
        }
        topics["__META__"]["ORDER"].append(bname)

    for topic in topics["__META__"]["ORDER"]:
        path = topics[topic]["path"]

        with open(path, "rb") as csv_fd:
            csv_reader = csv.reader(csv_fd)
            raw = list(csv_reader)

            topics[topic]["luns"] = [int(lun) for lun, sample in raw]
            topics[topic]["samples"] = [float(sample) for lun, sample in raw]

            sz = float(16777216)
            mb = (1024 * 1024)

            topics[topic]["throughput"] = [
                ((sz * lun) / wc)/mb for lun, wc in zip(
                    topics[topic]["luns"], topics[topic]["samples"]
                )
            ]
            topics[topic]["wall-clock"] = [
                sample * 1000 for sample in topics[topic]["samples"]
            ]

    plot_scale(topics, args)

if __name__ == "__main__":
    PRSR  = argparse.ArgumentParser(description="Visualize results")
    PRSR.add_argument(
        "result",
        nargs="+",
        help="Path to csv file containing results labeled by file basename"
    )
    PRSR.add_argument(
        "--title",
        type=str,
        default="Throughput|Wall-clock as a function of LUNs",
        help="Title of bar chart"
    )
    PRSR.add_argument(
        "--yaxis",
        type=str,
        default="throughput",
        help="Y-Axis values",
        choices=["throughput", "wall-clock"]
    )
    PRSR.add_argument(
        "--output",
        type=str,
        help="Name of chart file"
    )

    ARGS = PRSR.parse_args()
    main(ARGS)
