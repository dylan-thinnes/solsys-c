def clean(x):
    {
        type: x,
        children: .factors | map(
            (.spacer | objects | clean(0)),
            (.power  | objects | clean(1))
        )
    };

clean(1)
